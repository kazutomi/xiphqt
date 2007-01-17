/*
    Copyright (C) 2004 Kor Nielsen
    Pulse driver code copyright (C) 2006 Lennart Poettering
    Unholy union copyright (C) 2007 Monty and Redhat Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "oss2pulse.h"

static void close_helper(fd_info *i, int retval){
  if(i != NULL){
    debug(DEBUG_LEVEL_NORMAL, __FILE__": close_helper()\n");
    
    if(i->read_file){
      fusd_return(i->read_file, retval);
      i->read_file = NULL;
      i->read_buffer = NULL;
    }
    
    if(i->write_file){
      fusd_return(i->write_file, retval);
      i->write_file = NULL;
      i->write_buffer = NULL;
    }
    
    if(i->poll_file){
      fusd_return(i->poll_file, FUSD_NOTIFY_EXCEPT);
      i->poll_file = NULL;
    }

    i->unusable = 1;
  }
}

static void poll_notify(struct fusd_file_info* file, unsigned int cached_state){
  int state = 0;
  fd_info *i = file->private_data;
  if(i == NULL) return;
  if(i->unusable) return;

  if(!i->poll_file) return;

  if (i->play_stream && // active stream
      pa_stream_get_state(i->play_stream) == PA_STREAM_READY && // stream is ready
      !i->write_file){ // nothing already pending

    size_t n = pa_stream_writable_size(i->play_stream);
      
    // OSS selected upon a full fragment of space
    if (n >= i->fragment_size)
      state |= FUSD_NOTIFY_OUTPUT;
  }

  if (i->rec_stream && // active stream
      pa_stream_get_state(i->rec_stream) == PA_STREAM_READY && // stream is ready
      !i->read_file){ // nothing already pending
    
    ssize_t n = pa_stream_readable_size(i->rec_stream);
    
    if (n - i->read_offset > 0)
      state |= FUSD_NOTIFY_INPUT;
  }

  if(state != cached_state){
    fusd_return(i->poll_file, state);
    i->poll_file = NULL;
  }
}

static void stream_success_cb(pa_stream *s, int success, void *userdata) {
    fd_info *i = userdata;

    assert(s);
    assert(i);

    i->operation_success = success;
    pa_threaded_mainloop_signal(i->mainloop, 0);
}

static void fix_metrics(fd_info *i) {
    size_t fs;
    char t[PA_SAMPLE_SPEC_SNPRINT_MAX];

    fs = pa_frame_size(&i->sample_spec);

    /* Don't fix things more than necessary */
    if ((i->fragment_size % fs) == 0 &&
        i->n_fragments >= 2 &&
        i->fragment_size > 0)
        return;

    i->fragment_size = (i->fragment_size/fs)*fs;

    /* Number of fragments set? */
    if (i->n_fragments < 2) {
        if (i->fragment_size > 0) {
            i->n_fragments = pa_bytes_per_second(&i->sample_spec) / 2 / i->fragment_size;
            if (i->n_fragments < 2)
                i->n_fragments = 2;
        } else
            i->n_fragments = 12;
    }

    /* Fragment size set? */
    if (i->fragment_size <= 0) {
        i->fragment_size = pa_bytes_per_second(&i->sample_spec) / 2 / i->n_fragments;
        if (i->fragment_size < 1024)
            i->fragment_size = 1024;
    }

    debug(DEBUG_LEVEL_NORMAL, __FILE__": sample spec: %s\n", 
	  pa_sample_spec_snprint(t, sizeof(t), &i->sample_spec));
    debug(DEBUG_LEVEL_NORMAL, __FILE__": fixated metrics to %i fragments, %li bytes each.\n", 
	  i->n_fragments, (long)i->fragment_size);
}

static void stream_latency_update_cb(pa_stream *s, void *userdata) {
  fd_info *i = userdata;
  if(i == NULL) return;
  if(i->unusable) return;
  assert(s);
  
  pa_threaded_mainloop_signal(i->mainloop, 0);
}

static int stream_write_copy_data (fd_info *i) {
  if(i == NULL) return;
  if(i->unusable) return;

  if (i->write_file){

    if((i->play_stream) && 
       (pa_stream_get_state(i->play_stream) == PA_STREAM_READY)) {
      
      size_t n = pa_stream_writable_size(i->play_stream);
      
      if (n == (size_t)-1) {
	debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_writable_size(): %s\n",
	      pa_strerror(pa_context_errno(i->context)));
	goto fail;
      }
      
      while ( n > 0 && 
	      i->write_rem > 0 &&
	      (n >= i->fragment_size || i->write_rem < i->fragment_size)) {
	ssize_t r = i->fragment_size;
	if (r > i->write_rem) r = i->write_rem;
	if (r > n) r = n;
	
	if (pa_stream_write(i->play_stream, i->write_buffer, r, NULL, 0, PA_SEEK_RELATIVE) < 0) {
	  debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_write(): %s\n", pa_strerror(pa_context_errno(i->context)));
	  goto fail;
	}
	
	i->write_buffer += r;
	i->write_rem -= r;
	n -= r;
      }
    }

    if(i->write_rem <= 0){
      fusd_return(i->write_file, i->write_size);
      i->write_buffer = NULL;
      i->write_size = 0;
      i->write_rem = 0;
      i->write_file = NULL;
    }else{
      if(i->write_file->flags & O_NONBLOCK){
	/* nonblocking; have to return something now */
	int ret = i->write_size - i->write_rem;
	if(ret <= 0) ret= -EAGAIN;
	fusd_return(i->write_file, ret);

	i->write_buffer = NULL;
	i->write_size = 0;
	i->write_rem = 0;
	i->write_file = NULL;
      }
    }	
  }
  
  return 0;

 fail:
  if (i->write_file){
    i->write_buffer = NULL;
    i->write_size = 0;
    i->write_rem = 0;
    fusd_return(i->write_file, -EIO);
    i->write_file = NULL;
  }
  return -1;
}

static int stream_read_copy_data (fd_info *i) {
  if(i == NULL) return;
  if(i->unusable) return;

  if (i->read_file){
  
    if ((i->rec_stream) && 
	(pa_stream_get_state(i->rec_stream) == PA_STREAM_READY)) {

      ssize_t n = pa_stream_readable_size(i->rec_stream);
    
      if (n == (size_t)-1) {
	debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_readable_size(): %s\n",
	      pa_strerror(pa_context_errno(i->context)));
	goto fail;
      }
    
      while (n - i->read_offset > 0 && i->read_rem > 0){

	const char *data;
	size_t r;
	
	if (pa_stream_peek(i->rec_stream, (void *)&data, &r) < 0 || !data) {
	  debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_peek(): %s\n", pa_strerror(pa_context_errno(i->context)));
	  goto fail;
	}
      
	r -= i->read_offset;
	if(r > i->read_rem) r = i->read_rem;
	memcpy(i->read_buffer, data + i->read_offset, r);
	
	i->read_buffer += r;
	i->read_offset += r;
	i->read_rem -= r;

	if (i->read_offset >= i->fragment_size) {
	  if (pa_stream_drop(i->rec_stream) < 0) {
	    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_drop(): %s\n", pa_strerror(pa_context_errno(i->context)));
	    goto fail;
	  }
	  i->read_offset = 0;
	}
	
	assert(n >= (size_t) r);
	n -= r;
      }
    }
    if(i->read_rem <= 0){
      fusd_return(i->read_file, i->read_size);
      i->read_buffer = NULL;
      i->read_size = 0;
      i->read_rem = 0;
      i->read_file = NULL;
    }else{
      if(i->read_file->flags & O_NONBLOCK){
	/* nonblocking; have to return something now */
	int ret = i->read_size - i->read_rem;
	if(ret <= 0) ret= -EAGAIN;
	fusd_return(i->read_file, ret);

	i->read_buffer = NULL;
	i->read_size = 0;
	i->read_rem = 0;
	i->read_file = NULL;
      }
    }
  }
  return 0;

 fail:
  if (i->read_buffer){
    i->read_buffer = NULL;
    i->read_size = 0;
    i->read_rem = 0;
    fusd_return(i->read_file, -EIO);
    i->read_file = NULL;
  }
  return -1;
}

static void stream_request_cb(pa_stream *s, size_t length, void *userdata) {
  fd_info *i = userdata;
  if(i == NULL) return;
  if(i->unusable) return;

  assert(s);
  
  size_t n;
  if (s == i->play_stream) {
    n = pa_stream_writable_size(i->play_stream);
    if (n == (size_t)-1) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_writable_size(): %s\n",
	    pa_strerror(pa_context_errno(i->context)));
    }
    
    if (n >= i->fragment_size)
      stream_write_copy_data(i);
  }
  
  if (s == i->rec_stream) {
    n = pa_stream_readable_size(i->rec_stream);
    if (n == (size_t)-1) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_readable_size(): %s\n",
	    pa_strerror(pa_context_errno(i->context)));
    }
    
    if (n >= i->fragment_size)
      stream_read_copy_data(i);
  }

  if(i->poll_file)
    poll_notify(i->poll_file, fusd_get_poll_diff_cached_state(i->poll_file));

}

static void stream_state_cb(pa_stream *s, void *userdata) {
  fd_info *i = userdata;   
  if(i == NULL) return;
  if(i->unusable) return;

  assert(s);
  
  switch (pa_stream_get_state(s)) {
    
  case PA_STREAM_READY:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": stream established.\n");
    break;
    
  case PA_STREAM_FAILED:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_connect_playback() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
    
  case PA_STREAM_TERMINATED:
  case PA_STREAM_UNCONNECTED:
    close_helper(i, -EIO);
    break;

  case PA_STREAM_CREATING:
    break;
  }
}

static int create_playback_stream(struct fusd_file_info* file){
  fd_info *i= file->private_data;
  pa_buffer_attr attr;
  int n;
  
  if(i == NULL) return -1;
  if(i->unusable) return -1;
  
  fix_metrics(i);
  
  if (!(i->play_stream = pa_stream_new(i->context, "Audio Stream", &i->sample_spec, NULL))) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_new() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  pa_stream_set_state_callback(i->play_stream, stream_state_cb, i);
  pa_stream_set_write_callback(i->play_stream, stream_request_cb, i);
  pa_stream_set_latency_update_callback(i->play_stream, stream_latency_update_cb, i);
  
  memset(&attr, 0, sizeof(attr));
  attr.maxlength = i->fragment_size * (i->n_fragments+1);
  attr.tlength = i->fragment_size * i->n_fragments;
  attr.prebuf = i->fragment_size;
  attr.minreq = i->fragment_size;
  
  if (pa_stream_connect_playback(i->play_stream, NULL, &attr, PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_AUTO_TIMING_UPDATE, NULL, NULL) < 0) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_connect_playback() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  return 0;
  
 fail:
  return -1;
}

static int create_record_stream(struct fusd_file_info* file){
  fd_info *i= file->private_data;
  pa_buffer_attr attr;
  int n;
  
  if(i == NULL) return -1;
  if(i->unusable) return -1;
  
  fix_metrics(i);
   
  if (!(i->rec_stream = pa_stream_new(i->context, "Audio Stream", &i->sample_spec, NULL))) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_new() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  pa_stream_set_state_callback(i->rec_stream, stream_state_cb, i);
  pa_stream_set_read_callback(i->rec_stream, stream_request_cb, i);
  pa_stream_set_latency_update_callback(i->rec_stream, stream_latency_update_cb, i);
  
  memset(&attr, 0, sizeof(attr));
  attr.maxlength = i->fragment_size * (i->n_fragments+1);
  attr.fragsize = i->fragment_size;
  
  if (pa_stream_connect_record(i->rec_stream, NULL, &attr, PA_STREAM_INTERPOLATE_TIMING|PA_STREAM_AUTO_TIMING_UPDATE) < 0) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_connect_playback() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  return 0;

 fail:
  return -1;
}

static void free_streams(fd_info *i) {
  if(i == NULL) return;
  
  if (i->play_stream) {
    pa_stream_disconnect(i->play_stream);
    pa_stream_unref(i->play_stream);
    i->play_stream = NULL;
  }
  
  if (i->rec_stream) {
    pa_stream_disconnect(i->rec_stream);
    pa_stream_unref(i->rec_stream);
    i->rec_stream = NULL;
  }
}

static int map_format(int *fmt, pa_sample_spec *ss) {
    
    switch (*fmt) {
        case AFMT_MU_LAW:
            ss->format = PA_SAMPLE_ULAW;
            break;
            
        case AFMT_A_LAW:
            ss->format = PA_SAMPLE_ALAW;
            break;
            
        case AFMT_S8:
            *fmt = AFMT_U8;
            /* fall through */
        case AFMT_U8:
            ss->format = PA_SAMPLE_U8;
            break;
            
        case AFMT_U16_BE:
            *fmt = AFMT_S16_BE;
            /* fall through */
        case AFMT_S16_BE:
            ss->format = PA_SAMPLE_S16BE;
            break;
            
        case AFMT_U16_LE:
            *fmt = AFMT_S16_LE;
            /* fall through */
        case AFMT_S16_LE:
            ss->format = PA_SAMPLE_S16LE;
            break;
            
        default:
            ss->format = PA_SAMPLE_S16NE;
            *fmt = AFMT_S16_NE;
            break;
    }

    return 0;
}

static int map_format_back(pa_sample_format_t format) {
    switch (format) {
        case PA_SAMPLE_S16LE: return AFMT_S16_LE;
        case PA_SAMPLE_S16BE: return AFMT_S16_BE;
        case PA_SAMPLE_ULAW: return AFMT_MU_LAW;
        case PA_SAMPLE_ALAW: return AFMT_A_LAW;
        case PA_SAMPLE_U8: return AFMT_U8;
        default:
            abort();
    }
}

// the mainloop_wait below is unfortunately unavoidable; see the
// comments for dsp_close.

// Only ever called from an async worker thread, so the mainloop_wait
// can't block fusd.
int dsp_drain(fd_info *i) {
  if(i == NULL) return -1;
  
  pa_operation *o = NULL;
  int r = -1;
  
  if (!i->mainloop)
    return 0;
  
  debug(DEBUG_LEVEL_NORMAL, __FILE__": Draining.\n");
  
  pa_threaded_mainloop_lock(i->mainloop);
  
  if (!i->play_stream)
    goto fail;
  
  debug(DEBUG_LEVEL_NORMAL, __FILE__": Really draining.\n");
  
  if (!(o = pa_stream_drain(i->play_stream, stream_success_cb, i))) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_drain(): %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  i->operation_success = 0;
  while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": check goto.\n");
    PLAYBACK_STREAM_CHECK_DEAD_GOTO(i, fail);
    debug(DEBUG_LEVEL_NORMAL, __FILE__": waiting.\n");
    pa_threaded_mainloop_wait(i->mainloop);
  }
  
  if (!i->operation_success) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_drain() 2: %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  r = 0;
  
 fail:
  
  if (o)
    pa_operation_unref(o);
  
  pa_threaded_mainloop_unlock(i->mainloop);
  
  return r;
}

// Only ever called from an async worker thread, so the mainloop_wait
// can't block fusd.
static int dsp_trigger(fd_info *i) {
  if(i == NULL) return -1;
  pa_operation *o = NULL;
  int r = -1;
  
  if (!i->play_stream)
    return 0;
  
  pa_threaded_mainloop_lock(i->mainloop);
  
  //if (dsp_empty_socket(i) < 0)
  //  goto fail;
  
  debug(DEBUG_LEVEL_NORMAL, __FILE__": Triggering.\n");
  
  if (!(o = pa_stream_trigger(i->play_stream, stream_success_cb, i))) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_trigger(): %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  i->operation_success = 0;
  while (!pa_operation_get_state(o) != PA_OPERATION_DONE) {
    PLAYBACK_STREAM_CHECK_DEAD_GOTO(i, fail);
    
    pa_threaded_mainloop_wait(i->mainloop);
  }
  
  if (!i->operation_success) {
    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_trigger(): %s\n", pa_strerror(pa_context_errno(i->context)));
    goto fail;
  }
  
  r = 0;
  
 fail:
  
  if (o)
    pa_operation_unref(o);
  
  pa_threaded_mainloop_unlock(i->mainloop);
  
  return 0;
}

// fd_info_new can block and the mainloop thread manipulation within
// can't be done from a callback.
static void *dsp_open_thread(void *arg){
  struct fusd_file_info* file = arg;
  fd_info *i;
  int ret = 0;
  int f;
  
  debug(DEBUG_LEVEL_NORMAL, __FILE__": dsp_open_worker()\n");
  
  if ((i = fd_info_new(FD_INFO_STREAM, &ret))){

    debug(DEBUG_LEVEL_NORMAL, __FILE__": dsp_open() succeeded\n");
    
    fd_info_add_to_list(i);
    fd_info_unref(i);
    file->private_data = i;
  }

  fusd_return(file, -ret);
  return 0;
}

static int dsp_open(struct fusd_file_info* file){
  pthread_t dummy;
  debug(DEBUG_LEVEL_NORMAL, __FILE__": dsp_open()\n");
  if(pthread_create(&dummy,NULL,dsp_open_thread,file))
    dsp_open_thread(file);
  return -FUSD_NOREPLY;
}

static ssize_t dsp_read(struct fusd_file_info* file, 
		    char* buffer, 
		    size_t size, 
		    loff_t* offset){

  struct fd_info* i = file->private_data;
  struct device_info* info = (struct device_info*) file->device_info;
  int ret;

  if(i == NULL) return -EBADFD;
  if(i->unusable) return -EBADFD;

  debug(DEBUG_LEVEL_DEBUG2, "Reading %d bytes from device..\n", size);
  
  pa_threaded_mainloop_lock(i->mainloop);
  
  if(!i->rec_stream)
    if (create_record_stream(file) < 0){
      pa_threaded_mainloop_unlock(i->mainloop);
      return -EIO;
    }

  i->read_buffer = buffer;
  i->read_size = size;
  i->read_rem = size;
  i->read_file = file;

  stream_read_copy_data(i);

  pa_threaded_mainloop_unlock(i->mainloop);
  return -FUSD_NOREPLY;
}

static ssize_t dsp_write(struct fusd_file_info *file, 
		     const char *buffer,
		     size_t size, 
		     loff_t *offset){

  struct fd_info* i = file->private_data;
  struct device_info* info = (struct device_info*) file->device_info;
  int ret;

  if(i == NULL) return -EBADFD;
  if(i->unusable) return -EBADFD;
  
  debug(DEBUG_LEVEL_DEBUG2, "Writing %d bytes to device..\n", size);
  
  pa_threaded_mainloop_lock(i->mainloop);
  
  if(!i->play_stream)
    if (create_playback_stream(file) < 0){
      pa_threaded_mainloop_unlock(i->mainloop);
      return -EIO;
    }

  i->write_buffer = buffer;
  i->write_size = size;
  i->write_rem = size;
  i->write_file = file;

  stream_write_copy_data(i);

  pa_threaded_mainloop_unlock(i->mainloop);
  return -FUSD_NOREPLY;
}

// There are a few ioctl()s that can block; normally, we could use
// completion callbacks, but at the time of this note there's at least
// one operation (dsp_drain) that has to be threaded anyway because
// it's used in close(), and the last step of a close() can't be
// called from a callback.  Given that the elegant solution of
// callbacks isn't usable in all cases, we might as well take the easy
// way out (a worker thread) with ioctl()s as well.

static void *dsp_ioctl_thread(void *arg){
  struct fusd_file_info* file = arg;
  struct fd_info* i = file->private_data;

  if(i == NULL || i->unusable){
    fusd_return(file, -EBADFD);
  }else{
    int request = i->ioctl_request;
    void *argp = i->ioctl_argp;
    int ret = 0;

    debug(DEBUG_LEVEL_NORMAL, __FILE__": ioctl_worker()\n");

    switch (request) {
    case SNDCTL_DSP_SETFMT: 
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_SETFMT: %i\n", *(int*) argp);
      
      pa_threaded_mainloop_lock(i->mainloop);
      
      if (*(int*) argp == AFMT_QUERY)
	*(int*) argp = map_format_back(i->sample_spec.format);
      else {
	map_format((int*) argp, &i->sample_spec);
	free_streams(i);
      }
      
      pa_threaded_mainloop_unlock(i->mainloop);
      break;
      
    case SNDCTL_DSP_SPEED: 
      {
	pa_sample_spec ss;
	int valid;
	char t[256];
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_SPEED: %i\n", *(int*) argp);
	
	pa_threaded_mainloop_lock(i->mainloop);
	
	ss = i->sample_spec;
	ss.rate = *(int*) argp;
	
	if ((valid = pa_sample_spec_valid(&ss))) {
	  i->sample_spec = ss;
	  free_streams(i);
	}
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": ss: %s\n", pa_sample_spec_snprint(t, sizeof(t), &i->sample_spec));
	
	pa_threaded_mainloop_unlock(i->mainloop);
	
	if (!valid) ret = -EINVAL;    
      }
      break;
      
    case SNDCTL_DSP_STEREO:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_STEREO: %i\n", *(int*) argp);
      
      pa_threaded_mainloop_lock(i->mainloop);
      
      i->sample_spec.channels = *(int*) argp ? 2 : 1;
      free_streams(i);
      
      pa_threaded_mainloop_unlock(i->mainloop);
      break;
      
    case SNDCTL_DSP_CHANNELS: 
      {
	pa_sample_spec ss;
	int valid;
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_CHANNELS: %i\n", *(int*) argp);
	
	pa_threaded_mainloop_lock(i->mainloop);
	
	ss = i->sample_spec;
	ss.channels = *(int*) argp;
	
	if ((valid = pa_sample_spec_valid(&ss))) {
	  i->sample_spec = ss;
	  free_streams(i);
	}
	
	pa_threaded_mainloop_unlock(i->mainloop);
	
	if (!valid) ret = -EINVAL;
      }
      break;
      
    case SNDCTL_DSP_GETBLKSIZE:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_GETBLKSIZE\n");
      
      pa_threaded_mainloop_lock(i->mainloop);
      
      fix_metrics(i);
      *(int*) argp = i->fragment_size;
      
      pa_threaded_mainloop_unlock(i->mainloop);
      
      break;
      
    case SNDCTL_DSP_SETFRAGMENT:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_SETFRAGMENT: 0x%8x\n", *(int*) argp);
      
      pa_threaded_mainloop_lock(i->mainloop);
      
      i->fragment_size = 1 << (*(int*) argp);
      i->n_fragments = (*(int*) argp) >> 16;
      
      /* 0x7FFF means that we can set whatever we like */
      if (i->n_fragments == 0x7FFF)
	i->n_fragments = 12;
      
      free_streams(i);
      
      pa_threaded_mainloop_unlock(i->mainloop);
      
      break;
      
    case SNDCTL_DSP_GETCAPS:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_CAPS\n");
      
      *(int*)  argp = DSP_CAP_DUPLEX | DSP_CAP_MULTI;
      break;
      
    case SNDCTL_DSP_GETODELAY: 
      {
	int l;
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_GETODELAY\n");
	
	pa_threaded_mainloop_lock(i->mainloop);
	
	*(int*) argp = 0;
	
	for (;;) {
	  pa_usec_t usec;
	  
	  PLAYBACK_STREAM_CHECK_DEAD_GOTO(i, exit_loop);
	  
	  if (pa_stream_get_latency(i->play_stream, &usec, NULL) >= 0) {
	    *(int*) argp = pa_usec_to_bytes(usec, &i->sample_spec);
	    break;
	  }
	  
	  if (pa_context_errno(i->context) != PA_ERR_NODATA) {
	    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_get_latency(): %s\n", pa_strerror(pa_context_errno(i->context)));
	    break;
	  }
	  
	  pa_threaded_mainloop_wait(i->mainloop);
	}
	
      exit_loop:
	
	pa_threaded_mainloop_unlock(i->mainloop);
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": ODELAY: %i\n", *(int*) argp);
      }

      break;
      
    case SNDCTL_DSP_RESET: 
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_RESET\n");
      
      pa_threaded_mainloop_lock(i->mainloop);
      
      free_streams(i);
      reset_params(i);
      
      pa_threaded_mainloop_unlock(i->mainloop);
      break;
      
    case SNDCTL_DSP_GETFMTS:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_GETFMTS\n");
      *(int*) argp = AFMT_MU_LAW|AFMT_A_LAW|AFMT_U8|AFMT_S16_LE|AFMT_S16_BE;
      break;
      
    case SNDCTL_DSP_POST:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_POST\n");
    
      if (dsp_trigger(i) < 0) ret = -EIO;
      break;
      
    case SNDCTL_DSP_SYNC: 
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_SYNC\n");
      if (dsp_drain(i) < 0) ret = -EIO;
      break;
      
    case SNDCTL_DSP_GETOSPACE:
      {
	audio_buf_info *bi = (audio_buf_info*) argp;
	int l = 0;
	size_t k = 0;
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_GETOSPACE\n");
	pa_threaded_mainloop_lock(i->mainloop);
	
	fix_metrics(i);
	
	if (i->play_stream) {
	  if ((k = pa_stream_writable_size(i->play_stream)) == (size_t) -1)
	    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_writable_size(): %s\n", pa_strerror(pa_context_errno(i->context)));
	} else
	  k = i->fragment_size * i->n_fragments;
	
	bi->bytes = k > (size_t) l ? k - l : 0;
	
	
	bi->fragsize = i->fragment_size;
	bi->fragstotal = i->n_fragments;
	bi->fragments = bi->bytes / bi->fragsize;
	
	pa_threaded_mainloop_unlock(i->mainloop);
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": fragsize=%i, fragstotal=%i, bytes=%i, fragments=%i\n", 
	      bi->fragsize, bi->fragstotal, bi->bytes, bi->fragments);
	
      }
      break;
      
    case SNDCTL_DSP_GETISPACE: 
      {
	audio_buf_info *bi = (audio_buf_info*) argp;
	int l = 0;
	size_t k = 0;
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": SNDCTL_DSP_GETISPACE\n");
	
	pa_threaded_mainloop_lock(i->mainloop);
	
	fix_metrics(i);
	
	if (i->rec_stream) {
	  if ((k = pa_stream_readable_size(i->rec_stream)) == (size_t) -1)
	    debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_stream_readable_size(): %s\n", pa_strerror(pa_context_errno(i->context)));
	} else
	  k = 0;
	
	bi->bytes = k + l;
	
	bi->fragsize = i->fragment_size;
	bi->fragstotal = i->n_fragments;
	bi->fragments = bi->bytes / bi->fragsize;
	
	pa_threaded_mainloop_unlock(i->mainloop);
	
	debug(DEBUG_LEVEL_NORMAL, __FILE__": fragsize=%i, fragstotal=%i, bytes=%i, fragments=%i\n", 
	      bi->fragsize, bi->fragstotal, bi->bytes, bi->fragments);
	
      }
      break;
      
    case SNDCTL_DSP_GETIPTR:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": invalid ioctl SNDCTL_DSP_GETIPTR\n");
      ret = -EINVAL;
      break;

    case SNDCTL_DSP_GETOPTR:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": invalid ioctl SNDCTL_DSP_GETOPTR\n");
      ret = -EINVAL;
      break;

    default:
      debug(DEBUG_LEVEL_NORMAL, __FILE__": unknown ioctl 0x%08lx\n", request);
      ret = -EINVAL;
      break;
    }
    
    fusd_return(file, ret);

  }
  return 0;
}

static int dsp_ioctl(struct fusd_file_info *file, int request, void *argp){
  struct fd_info* i = file->private_data;
  pthread_t dummy;

  if(i == NULL) return -EBADFD;
  if(i->unusable) return -EBADFD;

  i->ioctl_request = request;
  i->ioctl_argp = argp;
  
  if(pthread_create(&dummy,NULL,dsp_ioctl_thread,file))
    dsp_ioctl_thread(file);

  return -FUSD_NOREPLY;
}

static int dsp_mmap(struct fusd_file_info *file, 
		    int offset, 
		    size_t length, 
		    int flags, 
		    void** addr, 
		    size_t* out_length){
  fd_info *i = file->private_data;
  if(i == NULL) return -EBADFD;
  if(i->unusable) return -EBADFD;

  return -EINVAL;
}

static int dsp_polldiff(struct fusd_file_info* file, unsigned int cached_state){
  fd_info *i = file->private_data;
  if(i == NULL) return -EBADFD;
  if(i->unusable) return -EBADFD;

  debug(DEBUG_LEVEL_NORMAL, __FILE__": poll_diff()\n");
  
  if(i->poll_file != NULL){
    fusd_destroy(i->poll_file);
    i->poll_file = NULL;
  }
        
  i->poll_file = file;
  poll_notify(file, cached_state);
  return -FUSD_NOREPLY;
}

// we have to thread the close call because Pulse has painted us
// into an API corner.  The drain call takes a completion callback,
// but we can't complete the close there because
// pa_threaded_mainloop_stop may not be called from a callback.

static void *dsp_close_thread(void *arg){
  struct fusd_file_info* file = arg;
  debug(DEBUG_LEVEL_NORMAL, __FILE__": close_worker()\n");
  fd_info *i = file->private_data;
  if(i == NULL)
    fusd_return(file, -EBADFD);
  else{
    int ret = 0;
    close_helper(i,0);  
    if(dsp_drain(i)) ret = -EIO; 
    fd_info_remove_from_list(i);
    fusd_return(file, ret);
  }
  return 0;
}

static int dsp_close(struct fusd_file_info* file){
  pthread_t dummy;
  debug(DEBUG_LEVEL_NORMAL, __FILE__": close()\n");
  if(pthread_create(&dummy,NULL,dsp_close_thread,file))
    dsp_close_thread(file);
  return -FUSD_NOREPLY;
}


struct fusd_file_operations dsp_file_ops = 
{
  open: dsp_open,
  read: dsp_read,
  write: dsp_write,
  mmap: dsp_mmap,
  poll_diff: dsp_polldiff,
  ioctl: dsp_ioctl,
  close: dsp_close
};

