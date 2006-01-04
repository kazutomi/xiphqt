/* cfgparse.h
 * - setup, and global structures built from setup
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 * Copyright (c) 2002 Karl Heyes <karl@pts.tele2.co.uk>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __CFGPARSE_H__
#define __CFGPARSE_H__

#define USE_PIPES

typedef struct _config_tag config_t;

#include <sys/time.h>
#include <unistd.h>

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <shout/shout.h>
#include "thread/thread.h"


#include "inputmodule.h"

extern int realtime_enabled;

#ifndef HAVE_STRNDUP
char *strndup(const char *str, size_t len);
#endif

struct cfg_tag
{
    const char *name;
    int (*retrieve) (xmlNodePtr node, void *x);
    void *storage;
};



struct output_module
{
    int (*output_send) (struct output_module *mod, ogg_packet *op, unsigned samples);
    void (*output_clear) (struct output_module *mod);
    int disabled;
    struct output_state *parent;
    struct output_module *next;
    void *specific;
    int need_headers;
    ogg_stream_state os;
    ogg_packet *prev_packet;
    int initial_granulepos;
    long serial;
    int in_use;
    long packetno;
    ogg_int64_t granule, start_pos;
    int initial_packets;
    int reset;
};



struct output_state
{
    struct output_module *head;

    long serial;
    int in_use;
    int headers;
    ogg_packet packets[3];
    int new_headers;
    ogg_int64_t granules, start_pos;
    unsigned granule_overlap;

    char *name;
    char *genre;
    char *description;
    char *url;

    ogg_stream_state in_os;
    vorbis_info  vi;
    vorbis_comment  vc;
    int info_in_use;
};



struct _config_tag
{
    int  background;
    int  realtime;
    char *pidfile;
    char *logpath;
    char *logfile;
    int  loglevel;
    unsigned logsize;
    int  log_stderr;
    char *user;
    const char *cfgfile;

    /* <metadata> */
    char *stream_name;
    char *stream_genre;
    char *stream_description;
    char *stream_url;

    input_module_t *inputs;

    unsigned runner_count;
    struct runner *runners;

    /* private */
    int log_id;
    int shutdown;
    int next_track;
    int has_encoding;
    int input_once_thru;

    struct _config_tag *next;
};



extern int get_xml_string (xmlNodePtr node, void *x);
extern int get_xml_int (xmlNodePtr node, void *x);
extern int get_xml_float (xmlNodePtr node, void *x);
extern int get_xml_bool (xmlNodePtr node, void *x);

int parse_xml_tags (const char *id, xmlNodePtr node, const struct cfg_tag *args);

static __inline__ int get_xml_param_name (xmlNodePtr node, char *p, char **where)
{
    char *tmp = (char *)xmlGetProp(node, (void*)p);
    if (tmp == NULL)
        return -1;
    *where = tmp;
    return 0;
}


extern config_t *ices_config;

void config_initialize(void);
void config_shutdown(void);

int config_read(const char *filename);
void config_dump(void);


#endif /* __CFGPARSE_H__ */





