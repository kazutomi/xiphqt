/* inputmodule.h
 * - the interface for input modules to implement.
 *
 * $Id: inputmodule.h,v 1.2 2001/09/25 12:04:21 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __INPUTMODULE_H__
#define __INPUTMODULE_H__

#include <vorbis/codec.h>

#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

typedef struct _input_module_tag input_module_t;
typedef struct _timing_control_tag  timing_control;
typedef struct _input_buffer_tag input_buffer;
typedef struct _module_param_tag module_param_t;
typedef struct _module module_t;

#define DEAD_AIR_BYTES  16384
extern char dead_audio [DEAD_AIR_BYTES];

typedef enum {
    ICES_INPUT_NONE = 0,
    ICES_INPUT_PCM,
    ICES_INPUT_VORBIS,
    ICES_INPUT_VORBIS_PACKET
    /* Can add others here in the future, if we want */
} input_type;

typedef enum _input_subtype {
    INPUT_SUBTYPE_INVALID,
    INPUT_PCM_LE_16,
    INPUT_PCM_BE_16,
    INPUT_PCM_UNINTERLEAVE
} input_subtype;


#define MIN_INPUT_BUFFERS 12

struct _input_buffer_tag
{
    uint64_t serial;
    void *buf;          /* could be ogg or pcm data */
    /* unsigned len; */

    int critical;
    int eos;

    /* long aux_data; */
    input_type type;
    input_subtype subtype;
    char **metadata;
    input_module_t *mod;

    input_buffer *next;         /* for list work */

    unsigned samplerate;
    unsigned channels;
    unsigned samples;
};



struct _timing_control_tag
{
    uint64_t starttime;
    uint64_t senttime;
    uint64_t oldsamples;
    int samples;
    int samplerate;
    int channels;
};



struct _module_param_tag
{
    char *name;
    char *value;

    module_param_t *next;
};


struct _input_module_tag
{
    module_param_t *module_params;
    char *module;

    unsigned id;
    const char *name;
    input_type type;
    input_subtype subtype;
    unsigned failures;
    time_t started;
    unsigned buffer_count;
    unsigned buffer_alloc;
    unsigned prealloc_count;
    unsigned delay_buffer_check;
    time_t start;

    int  (*getdata)(input_module_t *self);
    int  (*initialise_module)(input_module_t *);
    int  (*open_module)(input_module_t *);
    void (*close_module)(input_module_t *);
    void (*shutdown_module)(input_module_t *);
    void (*release_input_buffer)(input_module_t *, input_buffer *);
    void (*free_input_buffer)(input_module_t *, input_buffer *);
    int  (*initialise_buffer)(input_module_t *, input_buffer *);

    input_buffer   *free_list, **free_list_tail;
    uint64_t  expected;
    uint64_t  allotted_serial, released_serial;

    char *metadata_filename;

    void *internal; /* For the modules internal state data */

    input_module_t *next;
};


struct _module
{
    char *name;
    int  (*initialise)(input_module_t *);
    int  (*open)(input_module_t *);
    void (*close)(input_module_t *);
    void (*shutdown)(input_module_t *);
};


extern module_t modules[];

void input_sleep (void);
void input_free_buffer(input_buffer *);
input_buffer *input_alloc_buffer (input_module_t *);

void uninterleave_pcm_le (signed char  *src, unsigned channels, unsigned samples, float **dest);
void uninterleave_pcm_be (signed char  *src, unsigned channels, unsigned samples, float **dest);

void input_adv_sleep (unsigned long adv);  /* advance time in uS */
#if 0
void calculate_pcm_sleep(unsigned len, unsigned rate);
int calculate_ogg_sleep(ogg_page *page);
#endif

void send_for_processing (input_module_t *mod, input_buffer *ib);
void *input_loop(void*);

#endif /* __INPUTMODULE_H__ */

