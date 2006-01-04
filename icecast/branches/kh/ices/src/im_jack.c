/* im_jack.c
 * - input from JACK applications
 *
 * $Id: im_jack.c,v 0.7.0 2004/03/06 18:30:30 j Exp $
 *
 *
 * (c) 2004 jan gerber <j@thing.net>,
 * based on im_alsa.c which is...
 * by Jason Chu <jchu@uvic.ca>, based
 * on im_oss.c which is...
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ogg/ogg.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signals.h>

#include "cfgparse.h"
#include "metadata.h"

#include "im_jack.h"

#define MODULE "input-jack/"
#include "logging.h"

int jack_callback_process (jack_nframes_t nframes, void *arg)
{
    int chn; 
    input_module_t *mod=arg;
    im_jack_state *s = mod->internal;
    unsigned to_write = sizeof (jack_default_audio_sample_t) * nframes;
    int problem = 0;

    /* Do nothing until we're ready to begin. Stop doing on exit. */
    if ((!s->can_process) || s->user_terminated) 
        return 0;

    /* copy data to ringbuffer; one per channel */
    for (chn = 0; chn < (s->channels); chn++)
    {	
        int len;
        len = jack_ringbuffer_write (s->rb[chn],
                jack_port_get_buffer (s->jack_ports[chn], nframes), to_write);
        if (len < to_write)
        {
            problem = 1;
            break;
        }
    }
    if (problem)
    {
        s->jack_shutdown = 1;
        LOG_ERROR0 ("ringbuffer full");
    }
    /* input_adv_sleep ((uint64_t)nframes * 1000000 / s->rate); */
    return 0;
}

static int jack_initialise_buffer (input_module_t *mod, input_buffer *ib)
{
    im_jack_state *s = mod->internal;
    int i;
    float **ptr;

    if ((ib->buf = calloc (s->channels, sizeof (float*))) == NULL)
        return -1;
    ptr = ib->buf;
    for (i=s->channels ; i; i--)
    {
        if ((*ptr = calloc (sizeof(float), s->samples)) == NULL)
            return -1;
        ptr++;
    }
    ib->type = ICES_INPUT_PCM;
    ib->subtype = INPUT_PCM_LE_16;
    ib->critical = 0;
    ib->channels = s->channels;
    ib->samplerate = s->rate;
    ib->mod = mod;
    return 0;
}


void jack_close_module(input_module_t *mod)
{
    int i;
    if (mod)
    {
        LOG_DEBUG0 ("closing JACK module");
        /* input_sleep (); */
        if (mod->internal)
        {
            im_jack_state *s = mod->internal;
            if (s->client != NULL)
                jack_client_close(s->client);
            if(s->jack_ports)
                free(s->jack_ports);
            if(s->rb){
                for(i=0;i<s->channels;i++){
                    jack_ringbuffer_free(s->rb[i]);
                }
                free(s->rb);
                s->rb = NULL;
            }
        }
    }
}


void jack_shutdown_module (input_module_t *mod)
{
    im_jack_state *s = mod->internal;

    LOG_INFO0 ("Shutdown JACK module");
    free (s);
    mod->internal = NULL;
}

void jack_shutdown (void *arg)
{
    LOG_ERROR0("killed by JACK server");
    input_module_t *mod=arg;
    im_jack_state *s = mod->internal;
    s->jack_shutdown = 1;
}


/* 
   do the expensive stuff here so that
   jack_callback_process is not stopped by jack
 */
static int jack_read(input_module_t *mod) 
{
    im_jack_state *s = mod->internal;

    size_t i;
    int chn;
    input_buffer *ib;	
    jack_default_audio_sample_t **pcms;
    jack_nframes_t nframes=s->samples;
    size_t framebuf_size = sizeof (jack_default_audio_sample_t) * nframes;
    size_t fb_read;

    while (1)
    {
        /* read and process from ringbuffer has to go here. */
	fb_read=framebuf_size;
        if (jack_ringbuffer_read_space (s->rb[0]) > fb_read)
        {
            if ((ib = input_alloc_buffer (mod)) == NULL)
            {
                LOG_ERROR0 ("Failed buffer allocation");
                return 0;
            }
            pcms = ib->buf;

            for (chn = 0; chn < (s->channels); chn++)
            {	
                size_t len;
                len = jack_ringbuffer_read (s->rb[chn], (char*)pcms[chn], fb_read);
                if (len < fb_read)
                    fb_read = len;
            }

            ib->samples = fb_read/sizeof(jack_default_audio_sample_t);
            ib->samplerate = s->rate;
            ib->channels = s->channels;

            if (s->newtrack)
            {
                LOG_DEBUG0 ("metadata updates flagged");
                metadata_thread_signal (mod, ib);
                ib->critical = 1;
                s->newtrack = 0;
            }

            if (metadata_update_signalled || ices_config->shutdown || move_to_next_input)
            {
                LOG_DEBUG0("marking buffer eos");
                ib->eos = 1;
            }
            input_adv_sleep ((uint64_t)ib->samples * 1000000 / s->rate);
            send_for_processing (mod, ib);
        }
        else
        {
            thread_sleep (s->sleep);
        }
        if (metadata_update_signalled || ices_config->shutdown || move_to_next_input)
        {
            s->newtrack = 1;
            if (move_to_next_input || ices_config->shutdown)
                s->user_terminated = 1;
        }

        if (s->jack_shutdown)
        {
            s->jack_shutdown = 0;
            break;
        }
        if (s->user_terminated)
        {
            s->user_terminated = 0;
            break;
        }
    }
    return 0;
}

static void jack_return_buffer (input_module_t *mod __attribute__((unused)), input_buffer *ib)
{
    ib->critical = 0;
    ib->eos = 0;
    return;
}

int jack_init_module(input_module_t *mod)
{
    im_jack_state *s;
    module_param_t *current;
    char *clientname;
    char *connect;
    int channels, rate;
    unsigned int samples;
    unsigned sleep_time;

    mod->name = "JACK";
    mod->type = ICES_INPUT_PCM;
    mod->subtype = INPUT_PCM_LE_16;
    mod->getdata = jack_read;
    mod->release_input_buffer = jack_return_buffer;
    mod->initialise_buffer = jack_initialise_buffer;

    mod->internal = calloc(1, sizeof(im_jack_state));
    if (mod->internal == NULL)
        return -1;
    s = mod->internal;

    s->client = NULL; /* Set it to something invalid, for now */

    /* Defaults */
    rate = 48000; 
    channels = 2; 
    samples = 4096;
    sleep_time = 10000;
    clientname = "ices";
    connect = "";

    current = mod->module_params;
    while(current)
    {
        if(!strcmp(current->name, "channels"))
            channels = atoi(current->value);
        else if(!strcmp(current->name, "clientname"))
            clientname = current->value;
        else if(!strcmp(current->name, "metadatafilename"))
            mod->metadata_filename = current->value;
        else if(!strcmp(current->name, "sleep"))
            sleep_time = atoi (current->value);
	else if(!strcmp(current->name, "connect")) 
	    connect = current->value;
	else
            LOG_WARN1("Unknown parameter %s for jack module", current->name);

        current = current->next;
    }
    s->channels = channels;
    s->clientname = clientname;
    s->connect = connect;
    s->rate = rate;
    s->samples = samples;
    if (sleep_time > 0)
        s->sleep = sleep_time;

    /* allocate a few, so that mallocs don't kick in */
    mod->prealloc_count = 20 + (ices_config->runner_count * 5);
    mod->buffer_count = mod->prealloc_count;

    s->can_process=0;

    LOG_INFO0("JACK driver initialised");

    return 0;

}

int jack_open_module(input_module_t *mod)
{
    im_jack_state *s = mod->internal;
    int i,j;
    char port_name[32];
    size_t rb_size;
    const char *con_ptr=s->connect;
    const char *ind;
    int con_ports=0;
    int last=0;
    jack_port_t *input_port;
    const char *input_port_name;
    const char *output_port_name;
    char *connect[16];

    // Find out which ports to connect to.
    if(strcmp(s->connect, "")) {
        while (!last && con_ports < 16) {
            ind=index(con_ptr,',');
            if (ind==NULL) {
                ind=con_ptr+strlen(con_ptr);
                last=-1;
            }
            connect[con_ports] = strndup (con_ptr, (ind-con_ptr));
            //LOG_DEBUG1("Found port connect param: %s", connect[con_ports]);
            con_ptr=ind+1;
            con_ports++;
        }
    }

    if ((s->client = jack_client_new(s->clientname)) == 0) 
    {
        LOG_ERROR0("jack server not running");
        goto fail;
    }
    LOG_INFO2("Registering as %s:in_1,%s:in_2\n", s->clientname,s->clientname);

    /* do some memory management */
    s->jack_ports =(jack_port_t **)malloc(sizeof (jack_port_t *) * (s->channels));

    /* create the ringbuffers; one per channel, figures may need tweaking */
    rb_size = 2.0 * jack_get_sample_rate (s->client) * sizeof (jack_default_audio_sample_t);
    // why was with again(??)
    /* rb_size = (size_t)((s->sleep / 1000.0) *
        jack_get_buffer_size(s->client) * sizeof(jack_default_audio_sample_t));
     */
    LOG_DEBUG2("creating %d ringbuffers, one per channel, of "
            "%d bytes each", s->channels, rb_size);
    s->rb =(jack_ringbuffer_t **)malloc(sizeof (jack_ringbuffer_t *) * (s->channels));
    for(i=0;i<s->channels;i++){
        s->rb[i]=jack_ringbuffer_create(rb_size);
    }

    /* start the callback process */
    s->can_process=0; /* let the callback wait until all is running */
    jack_set_process_callback(s->client, jack_callback_process, mod);

    jack_on_shutdown (s->client, jack_shutdown, mod);

    /* set samplerate and samples */
    if(jack_get_sample_rate(s->client)>0)
    {
        s->rate = jack_get_sample_rate(s->client);
    }
    if (jack_activate(s->client)) 
    {
        LOG_ERROR0("cannot activate client");
        goto fail;
    }

    /* Create and connect the jack ports */
    for (i = 0; i < s->channels; i++) 
    {
        snprintf (port_name, sizeof(port_name),"in_%d", i+1);
        s->jack_ports[i] = jack_port_register(s->client, port_name, 
                JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput,0); 
	
    }
    
    for (i = 0; i < con_ports ; i++)
    {
	int j=i%(s->channels);
	
	if ((input_port=jack_port_by_name(s->client, connect[i])) == 0)
	{
	    LOG_DEBUG1("Can't find port %s to connect to", connect[i]);
	    goto fail;
	}
	input_port_name=jack_port_name(input_port);
	output_port_name=jack_port_name(s->jack_ports[j]);
	if (jack_connect(s->client, input_port_name, output_port_name))
	{
	    LOG_DEBUG1("Can't connect port: %s",connect[i]);
	    goto fail;
	}
	LOG_INFO2("Connected port: %s to %s",input_port_name,output_port_name);
    }

    s->newtrack = 1;

    for (i=0; i<con_ports; i++)
        free (connect[i]);
    LOG_INFO2("Channels %d / Samplerate %d", s->channels, s->rate);
    s->can_process=1;
    return 0;

fail:
    for (i=0; i<con_ports; i++)
        free (connect[i]);
    jack_shutdown_module(mod); /* safe, this checks for valid contents */
    return -1;
}
