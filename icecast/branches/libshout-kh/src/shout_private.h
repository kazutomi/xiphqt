/* shout.h: Private libshout data structures and declarations 
 * 
 * Copyright(c) 2003 Karl Heyes <karl@pts.tele2.co.uk>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; See the file COPYING
 *
 * if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __LIBSHOUT_SHOUT_PRIVATE_H__
#define __LIBSHOUT_SHOUT_PRIVATE_H__

#include <shout/shout.h>
#include <net/sock.h>
#include <timing/timing.h>
#include "util.h"

#include <sys/types.h>
#ifdef HAVE_STDINT_H
#  include <stdint.h>
#elif defined (HAVE_INTTYPES_H)
#  include <inttypes.h>
#endif


/* anymore than this is silly */
#define MAX_HTTP_SENT       4096
#define MAX_HTTP_RESPONSE   1024


#define LIBSHOUT_DEFAULT_HOST "localhost"
#define LIBSHOUT_DEFAULT_PORT 8000
#define LIBSHOUT_DEFAULT_FORMAT SHOUT_FORMAT_VORBIS
#define LIBSHOUT_DEFAULT_PROTOCOL SHOUT_PROTOCOL_HTTP
#define LIBSHOUT_DEFAULT_USER "source"
#define LIBSHOUT_DEFAULT_USERAGENT "libshout/" VERSION

typedef enum
{
    SHOUT_NOCONNECT,
    SHOUT_CONNECTED,
    SHOUT_REQ_MADE,
    SHOUT_REQ_SENT,
    SHOUT_READY
} shout_state_t;

struct shout {
    /* hostname or IP of icecast server */
    char *host;
    /* port of the icecast server */
    int port;
    /* login password for the server */
    char *password;
    /* server protocol to use */
    unsigned int protocol;
    /* type of data being sent */
    unsigned int format;
    /* audio encoding parameters */
    util_dict *audio_info;

    /* user-agent to use when doing HTTP login */
    char *useragent;
    /* mountpoint for this stream */
    char *mount;
    /* name of the stream */
    char *name;
    /* homepage of the stream */
    char *url;
    /* genre of the stream */
    char *genre;
    /* description of the stream */
    char *description;
    /* icecast 1.x dumpfile */
    char *dumpfile;
    /* username to use for HTTP auth. */
    char *user;
    /* bitrate of this stream */
    int bitrate;
    /* is this stream private? */
    int public;

    /* socket the connection is on */
    sock_t socket;

    /* timeout for reading sockets in blocking mode */
    unsigned read_timeout;

    void *format_data;
    int (*send)(shout_t* self, const void * buff, unsigned len);
    void (*close)(shout_t* self);
    int (*get_response)(shout_t *self);
    int (*format_open)(shout_t *self);
    int (*create_request)(shout_t *self);

    const char *mime_type;

    /* start of this period's timeclock */
    uint64_t starttime;
    /* amount of data we've sent (in milliseconds) */
    uint64_t senttime;

    int error;

    /* general state information */
    shout_state_t  state;
    int nonblocking;
    int pending_connection;

    signed char *send_header;
    int send_header_size;
    int send_header_offset;

    unsigned char *response;
    unsigned response_len;
    time_t response_senttime;

    unsigned pending_total;
    unsigned pending_limit;
    struct shout_pending_node *pending_queue, **pending_queue_tail;
};


struct shout_metadata {
	char *name;
	char *value;
	shout_metadata_t *next;
};

struct shout_pending_node
{
    void *data;   /* actual data start */
    void *start;  /* somewhere in data */
    unsigned len;
    struct shout_pending_node *next;
};

static __inline__ int send_header_write (shout_t *self, int count)
{
    if (count < 0)
        return 1;

    self->send_header_size += count;
    if (self->send_header_size >= MAX_HTTP_SENT)
        return 1;
    return 0;
}

static __inline__ signed char *send_header_end (shout_t *self)
{
    return self->send_header + self->send_header_size;
}

static __inline__ int send_header_remaining (shout_t *self)
{
    return MAX_HTTP_SENT - self->send_header_size;
}

#ifdef HAVE_VA_ARGS
#define add_send_header(x, ...)  send_header_write((x),snprintf(send_header_end(x) , send_header_remaining(x) , __VA_ARGS__))
#else
static __inline__ int add_send_header (shout_t *self, const char *fmt, ...)
{
    char *start = send_header_end (self);
    unsigned len = send_header_remaining (self);
    int ret;
    va_list ap;
    va_start (ap, fmt);
    ret = send_header_write (self, vsnprintf (start, len, fmt, ap));
    va_end (ap);
    return ret;
}
#endif




int shout_open_vorbis(shout_t *self);
int shout_open_mp3(shout_t *self);
int shout_queue_raw (shout_t *self, const void *data, size_t len);
int shout_write_direct (shout_t *self, const struct iovec *vecs, size_t count);
int shout_store_vec (shout_t *self, const struct iovec *vecs, size_t count, size_t written);


#endif /* __LIBSHOUT_SHOUT_PRIVATE_H__ */
