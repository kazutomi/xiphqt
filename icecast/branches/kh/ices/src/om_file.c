/* om_file.c
 *
 * output module for writing stream to file
 *
 * Copyright (c) 2003 Karl Heyes <karl@xiph.org>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#include <config.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>

#include <ogg/ogg.h>

#include <cfgparse.h>
#define MODULE "om_file/"
#include <logging.h>
#include <om_file.h>

static int _write_to_file (int savefile, ogg_page *og)
{
    struct iovec iov[2];
    iov[0].iov_base = (void*)og->header;
    iov[0].iov_len = og->header_len;
    iov[1].iov_base = (void*)og->body;
    iov[1].iov_len = og->body_len;
    if (ogg_page_granulepos(og) == -1)
        LOG_DEBUG1 ("granulepos is negative on page %ld", ogg_page_pageno(og)); 
    if (writev (savefile, iov, 2) == -1)
    {
        LOG_ERROR1 ("Failed to write to save file %s", strerror (errno));
        return -1;
    };
    return 0;
}


static void _store_last_packet (struct output_file_stream *file, ogg_packet *packet)
{
    /* copy the last packet on the file, so that it can be used as the first packet*/
    /* of the next, vorbis packets are interleaved so this prevent s data loss */
    ogg_packet *store_op = &file->last_packet;

    if (store_op->packet)
    {
        free (store_op->packet);
        store_op->packet = NULL;
    }
    memcpy (store_op, packet,  sizeof (file->last_packet));
    if ((store_op->packet = malloc (store_op->bytes)))
    {
        memcpy (store_op->packet, packet->packet, packet->bytes);
    }
    if (store_op->granulepos < 0)
        store_op->granulepos = 0;
        
    file->last_packet_exists = 1;
}


static void _flush_ogg_file (struct output_module *mod, ogg_packet *op)
{
    /* struct output_stream_state *out = &state->file; */
    struct output_file_stream *out = mod->specific;
    ogg_page page;
    int write_it = 1;

    long eos = op->e_o_s;

    op->e_o_s = 1; 
#ifdef DEBUG
    LOG_DEBUG0("marked packet for EOS");
#endif
    ogg_stream_packetin (&mod->os, op);

    while (ogg_stream_flush (&mod->os, &page) > 0)
    {
#ifdef DEBUG
        LOG_DEBUG1("flushing page (eos %d)", (ogg_page_eos (&page)?1:0));
#endif
        if (write_it && _write_to_file (out->fd, &page) < 0)
            write_it = 0;
    }
    ogg_stream_clear (&mod->os);
    /* reset to what it was */
    op->e_o_s = eos;
}


static int _output_file_create(struct output_module *mod, char *filename)
{
    char *start = filename;
    struct output_file_stream *stream = mod->specific;

    if (filename == NULL || filename[0] == '\0')
        return -1;
    if (filename[0] == '/')
        start++;

    while (1)
    {
        char *ptr;
        struct stat st;

        ptr = strchr (start, '/');
        if (ptr == NULL)
            break;
        *ptr = '\0';

#ifdef DEBUG
        LOG_DEBUG2 ("checking path %s (%s)", filename, start);
#endif
        if (stat (filename, &st) < 0)
        {
            char cwd[PATH_MAX];
            char *tmp_ptr;
            int ret = 0;

            if (errno != ENOENT)
            {
                LOG_ERROR2 ("unable to stat path %s (%s)", start, strerror (errno));
                return -1;
            }
            if (getcwd (cwd, sizeof (cwd)) == NULL)
            {
                LOG_ERROR0 ("failed to get cwd");
                return -1;
            }
            /* ok now create the directory, but we need to strip off the
             * new part first */
            if ((tmp_ptr = strrchr (filename, '/')))
                *tmp_ptr = '\0';
            if (tmp_ptr && filename[0] && chdir (filename) < 0)
            {
                LOG_ERROR1 ("chdir failed on %s", filename);
                if (tmp_ptr)
                    *tmp_ptr = '/';
                ret = -1;
            }
            else
            {
                if (mkdir (start, stream->dmask))
                {
                    LOG_ERROR2("Failed to mkdir %s in %s", start, filename);
                    if (tmp_ptr)
                        *tmp_ptr = '/';
                    ret = -1;
                }
                else
                {
                    if (tmp_ptr)
                        *tmp_ptr = '/';
                    LOG_INFO1 ("created directory %s", filename);
                }
            }
            if (chdir (cwd) < 0)
            {
                LOG_ERROR0 ("failed to reset cwd");
                ret = -1;
            }
            if (ret == -1)
                return -1;
        }
        start = ptr+1;
        *ptr = '/';
    }
    stream->fd = open (filename, O_CREAT|O_EXCL|O_WRONLY, stream->fmask);
    if (stream->fd == -1)
    {
        LOG_WARN2 ("Failed to open file %s (%s)", filename, strerror (errno));
        return -1;
    }
    LOG_INFO1 ("Saving to file %s", filename);
    return 0;
}


static int file_audio_pageout (struct output_module *mod, ogg_page *page)
{
    int ret;

    if (mod->initial_packets)
    {
        mod->initial_packets--;
        if (mod->initial_packets == 0)
            ret = ogg_stream_flush (&mod->os, page);
        else
            ret = 0;
    }
    else
        ret = ogg_stream_pageout (&mod->os, page);

    return ret;
}


/* return non-zero is packet flushed to file */
static int _output_file_timeout (struct output_module *mod, time_t time_val, ogg_packet *op)
{
    int ret = 0, close_it = 0;
    long packetno = op->packetno;
    struct output_state *state = mod->parent;
    struct output_file_stream *stream = mod->specific;

    if (stream->fd < 0)
        return 0;
    if (stream->close_on_new_headers && state->new_headers)
    {
        close_it = 1;
    }
    else
    {
        if (stream->saveduration)
        {
            if (stream->save_start + (time_t)stream->saveduration <= time_val)
            {
                ogg_int64_t  granule = op->granulepos;
                op->packetno = mod->packetno++;
                op->granulepos -= mod->start_pos;
                _flush_ogg_file (mod, op);
                op->granulepos = granule;
                close_it = 1;
                LOG_DEBUG0("file duration expired");
            }
        }
    }
    if (close_it)
    {
        LOG_DEBUG0 ("Closing save file");
        close (stream->fd);
        stream->fd = -1;
        _store_last_packet (stream, op);
        mod->packetno = 3;
        op->packetno = packetno;
        ret = 1;
    }
    return ret;
}


int output_ogg_file (struct output_module *mod, ogg_packet *op, unsigned samples)
{
    int output_needs_headers = 0;
    struct output_state *state = mod->parent;
    struct output_file_stream *file = mod->specific;
    ogg_page page;
    long packetno = op->packetno;
    ogg_int64_t granule = op->granulepos;
    time_t time_val;

    /* does the file need closing */
    time_val = time(NULL);   /* called far too much */
    if (_output_file_timeout (mod, time_val, op))
    {
        return 0;
    }
#ifdef DEBUG
    if (op->e_o_s)
        LOG_DEBUG0("packet seen with eos set");
#endif
    /* if no file open then open it */
    if (file->fd == -1)
    {
        struct tm tm;
        char filename [PATH_MAX];
        int ret;

        localtime_r (&time_val, &tm);
        if (file->savefilename)
        {
            ret = strftime (filename, sizeof (filename), file->savefilename, &tm);
            for (; ret && filename [ret-1]=='\\'; ret--)
                filename [ret-1] = '\0';
            if (ret == 0)
            {
                LOG_WARN1 ("Unable to generate a filename (%d)", ret);
                return -1;
            }
            LOG_DEBUG2 ("filename expansion %s -> %s", file->savefilename, filename);
        }
        else
            snprintf (filename, sizeof (filename), "save-%lu.ogg", file->count++);
        if (_output_file_create (mod, filename) < 0)
            return -1;

        file->save_start = time_val;
        output_needs_headers = 1;
    }
    if (state->new_headers || output_needs_headers)
    {
        ogg_page page;
        long packetno;
        int i;

        if (mod->in_use)
        {
            LOG_DEBUG0 ("clearing ogg file stream");
            ogg_stream_clear (&mod->os);
        }

        LOG_DEBUG0 ("initialising output stream");
        if (mod->reset)
            mod->start_pos = 0;
        mod->initial_packets = 2;
        mod->start_pos = 0;
        mod->in_use = 1;
        ogg_stream_init (&mod->os, ++mod->serial);
        mod->granule = (uint64_t)0;
        mod->packetno = 0; /* need to verify is start from 0 or 3 */
        for (i=0; i< 3 ; i++)
        {
            packetno = state->packets[i] . packetno;
            state->packets[i] . packetno = mod->packetno++;;
            /* LOG_DEBUG1 ("Added header %lld", state->packets[i] . packetno); */
            ogg_stream_packetin (&mod->os, &state->packets[i]);
        }

        while (ogg_stream_flush (&mod->os, &page) > 0)
        {
            /* LOG_DEBUG2 ("header: granulepos is %lld on page %ld\n", ogg_page_granulepos (&page), ogg_page_pageno(&page)); */
            if (file->fd > -1 && _write_to_file (file->fd, &page) < 0)
            {
                close (file->fd);
                file->fd = -1;
            }
        }
        output_needs_headers = 0;

        if (file->last_packet_exists)
        {
            file->last_packet.e_o_s = 0;
            if (mod->reset)
                mod->start_pos = file->last_packet.granulepos;
            file->last_packet.granulepos = 0;
            file->last_packet.packetno = mod->packetno++;
            /* LOG_DEBUG2 ("inital packet: granulepos is %lld on packet %ld", mod->last_packet.granulepos, mod->last_packet.packetno); */
            ogg_stream_packetin (&mod->os, &file->last_packet);
        }
        if (op->e_o_s)
            mod->initial_packets = 1;
    }

    op -> packetno = mod->packetno++;
    granule = op->granulepos;
    op->granulepos -= mod->start_pos;
    /* LOG_DEBUG2 ("main %lld granulepos is %lld", op->packetno, op->granulepos); */
    ogg_stream_packetin (&mod->os, op);

    while (file_audio_pageout (mod, &page) > 0)
    {
        if (file->fd > -1 && _write_to_file (file->fd, &page) < 0)
        {
            close (file->fd);
            file->fd = -1;
        }
    }
    /* reset packet to what it was */
    op->packetno = packetno;
    op->granulepos = granule;
    return 0;
}


static void output_filestream_clear (struct output_module *mod)
{
    if (mod)
    {
        struct output_file_stream *stream = mod->specific;
        LOG_DEBUG0("clearing structure");
        if (stream)
        {
            ogg_stream_clear (&mod->os);
            if (stream->fd != -1)
                close (stream->fd);
            if (stream->savefilename)
                xmlFree (stream->savefilename);
            if (stream->last_packet.packet)
                free (stream->last_packet.packet);
            free (stream);
            mod->specific = NULL;
        }
    }
}



int parse_savefile (xmlNodePtr node, void *arg)
{
    struct output_state *state = arg;
    char *filename = NULL;
    int duration = 60*60;   /* default to 1 hour */
    int single = 0;         /* enable to have a single logical stream per file */
    mode_t fmask = 0600;       /* file umask */
    mode_t dmask = 0700;       /* directory umask */
    int reset = 1;
    struct output_module *mod;

    mod = calloc (1, sizeof (struct output_module));
    while (mod)
    {
        struct cfg_tag  savefile_tags[] =
        {
            { "filename",       get_xml_string, &filename },
            { "duration",       get_xml_int,    &duration },
            { "on-metadata",    get_xml_bool,   &single },
            { "fmask",          get_xml_int,    &fmask },
            { "dmask",          get_xml_int,    &dmask },
            { "reset-time",     get_xml_bool,   &reset },
            { NULL, NULL, NULL }
        };
        struct output_file_stream *stream;

        if (parse_xml_tags ("savefile", node->xmlChildrenNode, savefile_tags))
            break;

        mod->parent = state;
        mod->output_send = output_ogg_file;
        mod->output_clear = output_filestream_clear;
        stream = calloc (1, sizeof (struct output_file_stream));
        if (stream == NULL)
            break;

        mod->reset = reset;
        mod->specific = stream;
        stream->fmask = fmask;
        stream->dmask = dmask;
        stream->close_on_new_headers = single;
        stream->savefilename = filename;
        stream->saveduration = duration;
        stream->fd = -1;

        mod->next = state->head;
        state->head = mod;
#ifdef DEBUG
        printf("Added file output stream\n");
#endif
        return 0;
    }
    if (mod) free (mod);
    if (filename) xmlFree (filename);

    return -1;
}

