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

static void fd_info_free(fd_info *i) {
    assert(i);

    debug(DEBUG_LEVEL_NORMAL, __FILE__": freeing fd info\n");

    dsp_drain(i);
    
    if (i->mainloop)
        pa_threaded_mainloop_stop(i->mainloop);
    
    if (i->play_stream) {
        pa_stream_disconnect(i->play_stream);
        pa_stream_unref(i->play_stream);
    }

    if (i->rec_stream) {
        pa_stream_disconnect(i->rec_stream);
        pa_stream_unref(i->rec_stream);
    }

    if (i->context) {
        pa_context_disconnect(i->context);
        pa_context_unref(i->context);
    }
    
    if (i->mainloop)
        pa_threaded_mainloop_free(i->mainloop);

    pthread_mutex_destroy(&i->mutex);
    free(i);
}

static fd_info *fd_info_ref(fd_info *i) {
    assert(i);
    
    pthread_mutex_lock(&i->mutex);
    assert(i->ref >= 1);
    i->ref++;

    debug(DEBUG_LEVEL_DEBUG2, __FILE__": ref++, now %i\n", i->ref);
    pthread_mutex_unlock(&i->mutex);

    return i;
}

void fd_info_unref(fd_info *i) {
    int r;
    pthread_mutex_lock(&i->mutex);
    assert(i->ref >= 1);
    r = --i->ref;
	debug(DEBUG_LEVEL_DEBUG2, __FILE__": ref--, now %i\n", i->ref);
    pthread_mutex_unlock(&i->mutex);

    if (r <= 0)
        fd_info_free(i);
}

static void context_state_cb(pa_context *c, void *userdata) {
    fd_info *i = userdata;
    assert(c);

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(i->mainloop, 0);
            break;

        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
    }
}

void reset_params(fd_info *i) {
    assert(i);
    
    i->sample_spec.format = PA_SAMPLE_U8;
    i->sample_spec.channels = 1;
    i->sample_spec.rate = 8000;
    i->fragment_size = 0;
    i->n_fragments = 0;
}


fd_info* fd_info_new(fd_info_type_t type, int *_errno) {
    fd_info *i;
    int sfds[2] = { -1, -1 };
    char name[64];
    static pthread_once_t install_atfork_once = PTHREAD_ONCE_INIT;

    debug(DEBUG_LEVEL_NORMAL, __FILE__": fd_info_new()\n");

    signal(SIGPIPE, SIG_IGN); /* Yes, ugly as hell */

    if (!(i = calloc(1,sizeof(*i)))) {
      *_errno = ENOMEM;
      goto fail;
    }
    
    i->type = type;

    pthread_mutex_init(&i->mutex, NULL);
    i->ref = 1;
    pa_cvolume_reset(&i->sink_volume, 2);
    pa_cvolume_reset(&i->source_volume, 2);
    i->volume_modify_count = 0;
    i->sink_index = (uint32_t) -1;
    i->source_index = (uint32_t) -1;
    PA_LLIST_INIT(fd_info, i);

    reset_params(i);

    if (!(i->mainloop = pa_threaded_mainloop_new())) {
        *_errno = EIO;
        debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_threaded_mainloop_new() failed\n");
        goto fail;
    }

    if (!(i->context = pa_context_new(pa_threaded_mainloop_get_api(i->mainloop), "PulseAudio OSS Emulation Daemon"))) {
        *_errno = EIO;
        debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_context_new() failed\n");
        goto fail;
    }

    pa_context_set_state_callback(i->context, context_state_cb, i);

    if (pa_context_connect(i->context, NULL, 0, NULL) < 0) {
        *_errno = ECONNREFUSED;
        debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
        goto fail;
    }

    pa_threaded_mainloop_lock(i->mainloop);

    if (pa_threaded_mainloop_start(i->mainloop) < 0) {
        *_errno = EIO;
        debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_threaded_mainloop_start() failed\n");
        goto unlock_and_fail;
    }

    /* Wait until the context is ready */
    pa_threaded_mainloop_wait(i->mainloop);

    if (pa_context_get_state(i->context) != PA_CONTEXT_READY) {
        *_errno = ECONNREFUSED;
        debug(DEBUG_LEVEL_NORMAL, __FILE__": pa_context_connect() failed: %s\n", pa_strerror(pa_context_errno(i->context)));
        goto unlock_and_fail;
    }

    pa_threaded_mainloop_unlock(i->mainloop);
    return i;

unlock_and_fail:

    pa_threaded_mainloop_unlock(i->mainloop);
    
fail:

    if (i)
        fd_info_unref(i);
    
    return NULL;
}

static pthread_mutex_t fd_infos_mutex = PTHREAD_MUTEX_INITIALIZER;
static PA_LLIST_HEAD(fd_info, fd_infos) = NULL;

void fd_info_add_to_list(fd_info *i) {
    assert(i);

    pthread_mutex_lock(&fd_infos_mutex);
    PA_LLIST_PREPEND(fd_info, fd_infos, i);
    pthread_mutex_unlock(&fd_infos_mutex);

    fd_info_ref(i);
}

void fd_info_remove_from_list(fd_info *i) {
    assert(i);

    pthread_mutex_lock(&fd_infos_mutex);
    PA_LLIST_REMOVE(fd_info, fd_infos, i);
    pthread_mutex_unlock(&fd_infos_mutex);

    fd_info_unref(i);
}

