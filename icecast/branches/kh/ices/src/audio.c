/* audio.c
 * stereo->mono downmixing
 * resampling
 *
 * $Id: audio.c,v 1.6 2003/03/15 02:24:18 karl Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cfgparse.h"
#include "audio.h"

#include "resample.h"

#define MODULE "audio/"
#include "logging.h"

struct downmix *downmix_initialise(void) {
    struct downmix *state = calloc(1, sizeof(struct downmix));

    LOG_INFO0("Enabling stereo->mono downmixing");

    return state;
}

void downmix_clear(struct downmix *s) {
    if(s) {
        if (s->buffer)
            free(s->buffer);
        free(s);
    }
}

void downmix_buffer_float(struct downmix *s, float **buf, int samples, unsigned channels)
{
    int i;

    if(samples > s->buflen) {
        void *tmp = realloc(s->buffer, samples * sizeof(float));
        if (tmp==NULL)
            return;
        s->buffer = tmp;
        s->buflen = samples;
    }

    for(i=0; i < samples; i++) {
        unsigned count = 0;
        float res = 0.0;
        for (count = 0; count < channels; count++)
            res += buf [count][i];
        s->buffer[i] = res/channels;
    }
    
}


#if 0
void downmix_buffer(struct downmix *s, signed char *buf, int len, int be)
{
    int samples = len/4;
    int i;

    if(samples > s->buflen) {
        void *tmp = realloc(s->buffer, samples * sizeof(float));
        if (tmp==NULL)
            return;
        s->buffer = tmp;
        s->buflen = samples;
    }

    if(be) {
        for(i=0; i < samples; i++) {
            s->buffer[i] = (((buf[4*i]<<8) | (buf[4*i + 1]&0xff)) +
                           ((buf[4*i + 2]<<8) | (buf[4*i + 3]&0xff)))/65536.f;
        }
    }
    else {
        for(i=0; i < samples; i++) {
            s->buffer[i] = (((buf[4*i + 1]<<8) | (buf[4*i]&0xff)) +
                           ((buf[4*i + 3]<<8) | (buf[4*i + 2]&0xff)))/65536.f;
        }
    }
}
#endif

struct resample *resample_initialise(int channels, int infreq, int outfreq)
{
    struct resample *state = calloc(1, sizeof(struct resample));
    int failed = 1;

    do 
    {
        if (state==NULL)
            break;
        if (resampler_init(&state->resampler, channels, outfreq, infreq, RES_END)) {
            LOG_ERROR0("Couldn't initialise resampler to specified frequency");
            return NULL;
        }

        if ((state->buffers = calloc(channels, sizeof(float *))) == NULL)
            break;
        if ((state->convbuf = calloc(channels, sizeof(float *))) == NULL)
            break;
        failed = 0;
    }
    while (0); /* not a loop */
        
    if (failed)
    {
        LOG_ERROR0("Couldn't initialise resampler due to memory allocation failure");
        resample_clear (state);
        return NULL;
    }
    state->channels = channels;

    LOG_INFO3("Initialised resampler for %d channels, from %d Hz to %d Hz",
            channels, infreq, outfreq);

    return state;
}

void resample_clear(struct resample *s)
{
    int c;

    if(s) {
        if(s->buffers) {
            for(c=0; c<s->channels; c++)
                if (s->buffers[c])
                    free(s->buffers[c]);
            free(s->buffers);
        }
        if(s->convbuf) {
            for(c=0; c<s->channels; c++)
                if (s->convbuf[c])
                    free(s->convbuf[c]);
            free(s->convbuf);
        }
        resampler_clear(&s->resampler);
        free(s);
    }
}

void resample_buffer(struct resample *s, signed char *buf, int buflen, int be)
{
    int c,i;
    buflen /= 2*s->channels; /* bytes -> samples conversion */

    if(s->convbuflen < buflen) {
        s->convbuflen = buflen;
        for(c=0; c < s->channels; c++)
            s->convbuf[c] = realloc(s->convbuf[c], buflen * sizeof(float));
    }

    if(be) {
        for(i=0; i < buflen; i++) {
            for(c=0; c < s->channels; c++) {
                s->convbuf[c][i] = ((buf[2*(i*s->channels + c)]<<8) |
                                    (0x00ff&(int)buf[2*(i*s->channels + c)+1]))/
                    32768.f;
            }
        }
    }
    else {
        for(i=0; i < buflen; i++) {
            for(c=0; c < s->channels; c++) {
                s->convbuf[c][i] = ((buf[2*(i*s->channels + c) + 1]<<8) |
                                    (0x00ff&(int)buf[2*(i*s->channels + c)]))/
                    32768.f;
            }
        }
    }

    resample_buffer_float(s, s->convbuf, buflen);
}

void resample_buffer_float(struct resample *s, float **buf, int buflen)
{
    int c;
    int res;

    s->buffill = resampler_push_check(&s->resampler, buflen);
    if(s->buffill <= 0) {
        LOG_ERROR1("Fatal reencoding error: resampler_push_check returned %d", 
                s->buffill);
    }

    if(s->bufsize < s->buffill) {
        s->bufsize = s->buffill;
        for(c=0; c<s->channels; c++)
            s->buffers[c] = realloc(s->buffers[c], s->bufsize * sizeof(float));
    }

    if((res = resampler_push(&s->resampler, s->buffers, (float const **)buf, buflen))
            != s->buffill) {
        LOG_ERROR2("Internal error in resampling: returned number of samples %d"
                   ", expected %d", res, s->buffill);
        s->buffill = res;
        return;
    }

}

void resample_finish(struct resample *s)
{
    int ret;

    if(!s->buffers[0])
        return;

    ret = resampler_drain(&s->resampler, s->buffers);

    if(ret > s->bufsize) {
        LOG_ERROR0("Fatal error in resampler: buffers too small");
        return;
    }

    s->buffill = ret;
}






