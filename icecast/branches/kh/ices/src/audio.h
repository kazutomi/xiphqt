/* audio.h
 * - stereo->mono downmixing
 * - resampling
 *
 * $Id: audio.h,v 1.3 2003/03/15 02:24:18 karl Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __AUDIO_H
#define __AUDIO_H

#include "resample.h"

struct downmix
{
    float *buffer;
    int buflen;
};

struct resample
{
    struct resampler resampler;
    int channels;

    float **buffers;
    int buffill;
    int bufsize;

    float **convbuf;
    int convbuflen;
};

struct downmix *downmix_initialise(void);
void downmix_clear(struct downmix *s);
/* void downmix_buffer(struct downmix *s, signed char *buf, int len, int be); */
void downmix_buffer_float(struct downmix *s, float **buf, int samples,  unsigned channels);

struct resample *resample_initialise(int channels, int infreq, int outfreq);
void resample_clear(struct resample *s);
void resample_buffer(struct resample *s, signed char *buf, int buflen, int be);
void resample_buffer_float(struct resample *s, float **buf, int buflen);
void resample_finish(struct resample *s);

#endif

