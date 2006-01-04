/* playlist.c
 * - Basic playlist functionality
 *
 * $Id: im_playlist.c,v 1.6 2002/08/03 15:05:38 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 * Copyright (c) 2003 Karl Heyes <karl@xiph.org>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ogg/ogg.h>

#include <thread/thread.h>

#include "cfgparse.h"
#include "stream.h"

#include "inputmodule.h"
#include "im_playlist.h"
#include "timing/timing.h"

#include "playlist_basic.h"
#include "metadata.h"
#include "signals.h"

#define MODULE "playlist-builtin/"
#include "logging.h"

#define OGG_BUFSIZE 4096
#define ALLOCATION_DELAY 500*1000  /* uS */

typedef struct _playlist_module 
{
    char *name;
    int (*init)(module_param_t *, struct playlist_state *);
} playlist_module_t;

static playlist_module_t pmodules[] = {
    { "basic", playlist_basic_initialise},
    { "script", playlist_script_initialise},
    {NULL,NULL}
};

static void playlist_clear_buffer (input_module_t *mod __attribute__((unused)), input_buffer *ib)
{
    if (ib)
    {
        ib->critical = 0;
        if (ib->buf)
        {
            ogg_packet *op = ib->buf;
            free (op->packet);
            free (op);
            ib->buf = NULL;
        }
    }
}

int playlist_open_module(input_module_t *mod)
{
    module_param_t *current;

    LOG_INFO0("open playlist module");

    current = mod->module_params;
    while (current)
    {
        if (!strcmp(current->name, "type"))
        {
            int current_module = 0;

            while(pmodules[current_module].init)
            {
                if(!strcmp(current->value, pmodules[current_module].name))
                {
                    struct playlist_state *pl = (struct playlist_state *)mod->internal;

                    if (pmodules[current_module].init (mod->module_params, pl))
                    {
                        LOG_ERROR0("Playlist initialisation failed");
                        return -1;
                    }
                    pl->filename = NULL;
                    pl->next_track = 1;
                    pl->terminate = 0;
                    pl->prev_op = NULL;
                    LOG_DEBUG0 ("initialised module");
                    return 0;
                }
                current_module++;
            }
            break;
        }
        current = current->next;
    }
    return -1;
}


static void close_file (struct playlist_state *pl)
{
    if (pl->current_file)
    {
        fclose (pl->current_file);
        pl->current_file = NULL;
        /* Reinit sync, so that dead data from previous file is discarded */
        ogg_stream_clear(&pl->os);
        ogg_sync_clear(&pl->oy);
    }
}


void playlist_close_module(input_module_t *mod)
{
    LOG_INFO0 ("Close playlist module");
    if (mod->internal) 
    {
        struct playlist_state *pl = (struct playlist_state *)mod->internal;
        pl->free_filename (pl->data, pl->filename);
        pl->clear (pl->data);
        pl->data = NULL;
        close_file (pl);
    }
}

void playlist_shutdown_module(input_module_t *mod)
{
    input_buffer *ib;

    LOG_INFO0 ("Shutdown playlist module");

    if (mod->internal) 
    {
        struct playlist_state *pl = (struct playlist_state *)mod->internal;
        if (pl->data) pl->clear(pl->data);
        ogg_sync_clear(&pl->oy);
        free(pl);
    }
    while (1)
    {
        ib = mod->free_list;
        if (ib == NULL)
            break;
        mod->free_list = ib->next;
        free (ib);
    }
}


static int find_next_entry (struct playlist_state *pl)
{
    char *newfn;

    pl->next_track = 0;

    if (ices_config->shutdown || pl->terminate)
        return 0;

    while (1)
    {
        if (pl->errors > 5) 
        {
            LOG_WARN0("Too many consecutive errors - exiting");
            return 0;
        }

        newfn = pl->get_filename(pl->data);
        if (newfn == NULL)
        {
            LOG_DEBUG0 ("no filename returned");
            return 0;  /* No more files available */
        }

        if (strcmp (newfn, "-"))
        {
            pl->current_file = fopen(newfn, "rb");
            if (pl->current_file == NULL)
            {
                LOG_WARN2("Error opening file \"%s\": %s", newfn, strerror(errno));
                pl->free_filename (pl->data, newfn);
                pl->errors++;
                continue;
            }
        }
        else
        {
            pl->current_file = stdin;
        }
        pl->errors = 0;

        break;
    }
    pl->free_filename(pl->data, pl->filename);
    pl->filename = newfn;

    LOG_INFO1("Currently playing \"%s\"", newfn);

    ogg_sync_init(&pl->oy);
    return 1;
}


static ogg_packet *copy_ogg_packet (ogg_packet *packet)
{
    ogg_packet *next;
    do
    {
        next = malloc (sizeof (ogg_packet));
        if (next == NULL)
            break;
        memcpy (next, packet, sizeof (ogg_packet));
        next->packet = malloc (next->bytes);
        if (next->packet == NULL)
            break;
        memcpy (next->packet, packet->packet, next->bytes);
        return next;
    } while (0);

    if (next)
        free (next);
    return NULL;
}


static void *alloc_ogg_buffer (input_module_t *mod, unsigned len)
{
    struct playlist_state *pl = (struct playlist_state *)mod->internal;
    return ogg_sync_buffer (&pl->oy, len);
}


static void prepare_buffer (input_buffer *ib, struct playlist_state *pl, int force_eos)
{
    ogg_packet *op = pl->prev_packet;

    ib->buf = op;
    if (op->b_o_s)
    {
        ib->critical = 1;
        vorbis_info_init (&pl->vi);
        vorbis_comment_init (&pl->vc);
        pl->more_headers = 3;
    }
    if (pl->more_headers)
    {
        if (vorbis_synthesis_headerin (&pl->vi, &pl->vc, op) < 0)
        {
            LOG_ERROR1("Problem with vorbis header %d", 4-pl->more_headers);
            metadata_update_signalled = 1;
            pl->prev_packet = NULL;
        }
        pl->more_headers--;
        pl->prev_window = 0;
        ib->samples = 0;
    }
    else
    {
        int window = vorbis_packet_blocksize (&pl->vi, op) / 4;
        if (pl->prev_window)
        {
            pl->granulepos += pl->prev_window + window;
            ib->samples = pl->prev_window + window;
        }
        else
        {
            pl->granulepos = 0;
            ib->samples = 0;
        }
        pl->prev_window = window;
        op->granulepos = pl->granulepos;
        ib->samplerate = pl->vi.rate;
        ib->channels = pl->vi.channels;
    }
    ib->type = ICES_INPUT_VORBIS_PACKET;
    if (force_eos || op->e_o_s)
    {
        ib->eos = 1;
        op->e_o_s = 1;
        vorbis_comment_clear (&pl->vc);
        vorbis_info_clear (&pl->vi);
    }
    /* printf ("packet has %ld granulepos\n", op->granulepos); */
    input_adv_sleep ((unsigned long)(ib->samples * 1000000.0 / ib->samplerate));
}


static void flush_file (input_module_t *mod)
{
    struct playlist_state *pl = (struct playlist_state *)mod->internal;

    /* flush final EOS packet */
    if (pl->prev_packet)
    {
        input_buffer *ib = input_alloc_buffer (mod);
        if (ib)
        {
            LOG_DEBUG0("Flushing EOS packet");
            prepare_buffer (ib, pl, 1);
            send_for_processing (mod, ib);
            pl->prev_packet = NULL;
        }
    }
    close_file(pl);
}


static int write_ogg_data (input_module_t *mod, void *buffer, unsigned len)
{
    struct playlist_state *pl = (struct playlist_state *)mod->internal;
    ogg_page page;
    
    ogg_sync_wrote (&pl->oy, len);
    /* data now in the stream, lets see about getting packets */
    while (1)
    {
        ogg_packet packet;
        int result;

        while (ogg_stream_packetpeek (&pl->os, &packet) > 0)
        {
            /* we cache one packet so that EOS can be marked even if it's missing */
            if (pl->prev_packet)
            {
                input_buffer *ib = NULL;
                int buffers_ok = 1;

                while (1)
                {
                    if (metadata_update_signalled)
                    {
                        LOG_INFO0("switching to next entry in playlist");
                        metadata_update_signalled = 0;
                        return 0;
                    }
                    if (ices_config->shutdown || move_to_next_input)
                    {
                        LOG_INFO0("module shutdown requested");
                        pl->terminate = 1;
                        return 0;
                    }
                    ib = input_alloc_buffer (mod);
                    if (ib)
                        break;

                    if (pl->sleep && buffers_ok)
                    {
                        LOG_ERROR0 ("failed buffer allocation");
                        buffers_ok = 0;
                    }

                    input_adv_sleep (ALLOCATION_DELAY);
                    input_sleep ();
                }
                /* pass BOS just in case EOS is missing in the input stream */
                prepare_buffer (ib, pl, packet.b_o_s);
                send_for_processing (mod, ib);
                if (metadata_update_signalled)
                    return 0;
            }
            /* copy the packet */
            pl->prev_packet = copy_ogg_packet (&packet);
            ogg_stream_packetout (&pl->os, NULL);
            if (pl->sleep)
                input_sleep();
        }
        result = ogg_sync_pageout (&pl->oy, &page);
        if (result < 0)
        {
            LOG_WARN1("Corrupt or missing data in stream from %s", pl->filename);
            break;
        }
        if (result == 0)
            break;
        /* ok, we have a page, now do we initialise the stream */
        if (ogg_page_bos (&page))
        {
            if (pl->os_init)
                ogg_stream_clear (&pl->os);
            ogg_stream_init (&pl->os, ogg_page_serialno (&page));
            pl->os_init = 1;
        }
        ogg_stream_pagein (&pl->os, &page);
    }
    return len;
}


/* Core streaming function for this module
 * This is what actually produces the data which gets streamed.
 *
 */

static int playlist_read(input_module_t *mod)
{
    struct playlist_state *pl = (struct playlist_state *)mod->internal;

    /* setup callbacks */
    pl->write_func = write_ogg_data;
    pl->alloc_buffer = alloc_ogg_buffer;
    pl->flush_func = flush_file;
    mod->release_input_buffer = playlist_clear_buffer;

    /* start processing */
    while (find_next_entry(pl))
    {
        while (1)
        {
            void *buffer;
            int len;

            buffer = pl->alloc_buffer (mod, OGG_BUFSIZE);
            len = fread (buffer, 1, OGG_BUFSIZE, pl->current_file);
            if (len == 0)
                break;

            if (pl->write_func (mod , buffer, len) < len)
                break;
        }
        LOG_DEBUG0 ("End of file");
        /* callback for flush */
        pl->flush_func (mod);
    }
    return 0;
}


static int playlist_init_buffer (input_module_t *mod, input_buffer *ib)
{
    ib->type = ICES_INPUT_VORBIS;
    ib->subtype = INPUT_SUBTYPE_INVALID;
    ib->critical = 0;
    ib->mod = mod;
    return 0;
}


int playlist_init_module (input_module_t *mod)
{
    LOG_INFO0 ("Initialise playlist module");
    mod->type = ICES_INPUT_VORBIS;
    mod->name = "playlist";
    mod->getdata = playlist_read;
    mod->initialise_buffer = playlist_init_buffer;
    if (mod->buffer_count == 0)
        mod->buffer_count = ices_config->runner_count*100 + 400;
    mod->prealloc_count = ices_config->runner_count * 30 + 40;

    mod->internal = calloc(1, sizeof(struct playlist_state));
    if (mod->internal == NULL)
        return -1;

    return 0;
}


