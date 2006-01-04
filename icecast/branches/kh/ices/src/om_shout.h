/* om_shout.h
 *
 * output module header for rebuild stream fro sending though libshout
 *
 * Copyright (c) 2002 Karl Heyes <karl@pts.tele2.co.uk>
 *
 * his program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __OM_SHOUT_H
#define __OM_SHOUT_H

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

struct output_shout_state
{
    shout_t *shout;
    char *hostname;
    char *user;
    char *password;
    char *mount;
    int port;
    int yp;
    int connected;
    int flush_trigger;
    int reconnect_delay;
    int reconnect_count;
    int reconnect_attempts;
    time_t restart_time;
    int page_samples;
    uint64_t prev_page_granulepos;
};

int parse_shout (xmlNodePtr node, void *arg);


#endif /* __OM_SHOUT_H */
