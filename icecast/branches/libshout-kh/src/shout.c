/* shout.c  Implementation of public libshout interface shout.h 
 * 
 * Copyright(c) 2003-4 Karl Heyes <karl@xiph.org>
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
 * License along with this library; see the file COPYING; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
 #include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <shout/shout.h>
#include <net/sock.h>
#include <timing/timing.h>
#include <httpp/httpp.h>
#include <thread/thread.h>

#include "shout_private.h"
#include "util.h"

/* -- local prototypes -- */
static char *http_basic_authorization(shout_t *self);

/* -- static data -- */
static int _initialized = 0;


static void shout_free_pending (shout_t *self)
{
    struct shout_pending_node *node = self->pending_queue, *next;

    while (node)
    {
        next = node->next;

        if (node->data)
            free (node->data);
        free (node);

        node = next;
    }
    self->pending_total = 0;
    self->pending_queue = NULL;
    self->pending_queue_tail = &self->pending_queue;
}


static int shout_try_write (shout_t *self, const void *data, unsigned len)
{
    int ret = sock_write_bytes (self->socket, data, len);

    if (ret < 0)
    {
        if (sock_recoverable (sock_error()))
        {
            self->error = SHOUTERR_BUSY;
            return 0;
        }
        self->error = SHOUTERR_SOCKET;
    }
    return ret;
}


/* -- static function definitions -- */
static char *http_basic_authorization(shout_t *self)
{
    char *out, *in;
    int len;

    if (!self || !self->user || !self->password)
        return NULL;

    len = strlen(self->user) + strlen(self->password) + 2;
    if (!(in = malloc(len)))
        return NULL;
    sprintf(in, "%s:%s", self->user, self->password);
    out = util_base64_encode(in);
    free(in);

    len = strlen(out) + 24;
    if (!(in = malloc(len))) {
        free(out);
        return NULL;
    }
    sprintf(in, "Authorization: Basic %s\r\n", out);
    free(out);

    return in;
}


static int shout_connect (shout_t *self)
{
    if (self->pending_connection)
    {
        int ret;

        self->error = SHOUTERR_PENDING;
        switch (sock_connected (self->socket, 0))
        {
            case 1:
                self->error = SHOUTERR_SUCCESS;
                self->state = SHOUT_CONNECTED;
                self->pending_connection = 0;
                return 0;
            case SOCK_ERROR:
                self->error = SHOUTERR_NOCONNECT;
                self->pending_connection = 0;
                /* fall thru */
            case SOCK_TIMEOUT:
            default: 
                return -1;
        }
    }

    if (self->nonblocking)
    {
        self->error = SHOUTERR_PENDING;
        self->socket = sock_connect_non_blocking (self->host, self->port);
        if (self->socket < 0)
            self->error = SHOUTERR_NOCONNECT;
        else
            self->pending_connection = 1;
        return -1;
    }
    else
    {
        self->error = SHOUTERR_NOCONNECT;
        self->socket = sock_connect(self->host, self->port);
        if (self->socket < 0)
            return -1;
        self->error = SHOUTERR_SUCCESS;
        self->state = SHOUT_CONNECTED;
        return 0;
    }
}


int shout_send_pending (shout_t *self)
{
    if (self->pending_queue == NULL)
        return 0;
    else
    {
        struct shout_pending_node *node = self->pending_queue;
        int written;

        while (node)
        {
            written = shout_try_write (self, node->start, node->len);
            if (written == (int)node->len)
            {
                self->pending_queue = node->next;
                if (self->pending_queue == NULL)
                    self->pending_queue_tail = &self->pending_queue;
                free (node->data);
                free (node);
                node = self->pending_queue;
                self->pending_total -= written;
            }
            else
            {
                if (written < 0)
                    return -1;

                node->start = (char*)node->start+written;
                node->len -= written;
                self->pending_total -= written;
                self->error = SHOUTERR_SUCCESS;
                break;
            }
        }
    }
    self->error = SHOUTERR_SUCCESS;

    return 0;
}


static int create_http_request (shout_t *self)
{
    int ret = -1;
    char *auth = NULL, *data = NULL;
    self->error = SHOUTERR_MALLOC;

    while (1)
    {
        if (add_send_header (self, "SOURCE %s HTTP/1.0\r\n", self->mount))
            break;

        if (add_send_header (self, "ice-name: %s\r\n", self->name != NULL ? self->name : "no name"))
            break;
        if (self->url)
            if (add_send_header (self, "ice-url: %s\r\n", self->url))
                break;
        if (self->genre)
            if (add_send_header (self, "ice-genre: %s\r\n", self->genre))
                break;
        if ((data = util_dict_urlencode(self->audio_info, ';')))
            if (add_send_header (self, "ice-audio-info: %s\r\n", data))
                break;
        if (add_send_header (self, "ice-public: %d\r\n", self->public))
            break;
        if (self->description)
            if (add_send_header (self, "ice-description: %s\r\n", self->description))
                break;
        if (self->useragent)
            if (add_send_header (self, "User-Agent: %s\r\n", self->useragent))
                break;
        if (add_send_header (self, "Content-Type: %s\r\n", self->mime_type))
            break;
        auth = http_basic_authorization (self);
        if (auth == NULL)
        {
            self->error = SHOUTERR_INSANE;
            break; /* need username and password */
        }
        else
        {
            if (add_send_header (self, "%s", auth))
                break;
        }
        if (add_send_header (self, "\r\n"))
            break;
        ret = 0;
        self->error = SHOUTERR_SUCCESS;
        break;
    }
    if (data)  free (data);

    return ret;
}


static int send_request(shout_t *self)
{
    switch (self->state)
    {
        case SHOUT_REQ_SENT:
            self->error = SHOUTERR_CONNECTED;
            return 0;

        case SHOUT_CONNECTED:
            if (self->create_request)
            {
                if (self->create_request (self) < 0)
                    return -1;
                self->state = SHOUT_REQ_MADE;
                /* ok, we've made the header */
            }
            else
                break;

        case SHOUT_REQ_MADE:
            if (shout_send_raw (self, self->send_header + self->send_header_offset,
                    self->send_header_size - self->send_header_offset) < 0)
                 return -1;
            break;
        default:
            self->error = SHOUTERR_INSANE;
            return -1;
    }
    self->state = SHOUT_REQ_SENT;
    self->response_senttime = time(NULL);
    return 0;
}



static int shout_read_response (shout_t *self)
{
    char *ptr;

    shout_send_pending (self);
    if (self->state == SHOUT_REQ_SENT)
    {
        self->error = SHOUTERR_SOCKET;
        while (1)
        {
            int bytes;
            char  *start;

            if (self->nonblocking == 0)
            {
                switch (sock_read_pending (self->socket, self->read_timeout*1000))
                {
                    case SOCK_ERROR:
                        self->error = SHOUTERR_SOCKET;
                        return -1;
                    case SOCK_TIMEOUT:
                        self->error = SHOUTERR_NOCONNECT;
                        return -1;
                    default:
                        break;
                }
            }
            else
            {
                time_t now = time (NULL);
                if ((time_t)(self->response_senttime + self->read_timeout) < now)
                {
                    self->error = SHOUTERR_NOCONNECT;
                    return -1;
                }
                self->error = SHOUTERR_PENDING;
                switch (sock_read_pending (self->socket, 0))
                {
                    case SOCK_ERROR:
                        self->error = SHOUTERR_SOCKET;
                    case SOCK_TIMEOUT:
                    case 0:
                        return -1;
                    default:
                        break;
                }
            }
            bytes = sock_read_bytes (self->socket, self->response + self->response_len,
                    MAX_HTTP_RESPONSE - self->response_len - 1);
            if (bytes == 0)
            {
                self->error = SHOUTERR_SOCKET;
                return -1;
            }
            if (bytes < 0)
            {
                if (sock_stalled (sock_error()))
                    self->error = SHOUTERR_PENDING;
                return -1;
            }

            /* lets get rid of any '\r' in '\r\n' sequences */
            start = ptr = (char *)(self->response + self->response_len);
            ptr [bytes] = '\0';
            while (1)
            {
                char * tmp_ptr;
                tmp_ptr = strstr (ptr, "\r\n");
                if (tmp_ptr)
                {
                    memmove (tmp_ptr, tmp_ptr+1, bytes-(tmp_ptr-ptr));
                    bytes--;  /* removed \r */
                    ptr = tmp_ptr+1;
                }
                else
                    break;
            }
            self->response_len += bytes;

            if (strstr (start, "\n\n"))
                break;
        }

        self->error = SHOUTERR_SUCCESS;
        return self->response_len;
    }
    self->error = SHOUTERR_INSANE;
    return -1;
}


static int shout_login (shout_t *self)
{
    if (self->state == SHOUT_NOCONNECT && shout_connect (self) < 0)
        return -1; 

    if (send_request (self) < 0)
        return -1;

    if (self->get_response (self) < 0)
        return -1;
    self->state = SHOUT_READY;
    return 0;
}


static int login_http(shout_t *self)
{
    http_parser_t *parser;
    int code;
    char *retcode; 
    
    if (shout_read_response (self) < 0)
        return -1;

    self->error = SHOUTERR_NOLOGIN;
    parser = httpp_create_parser();
    httpp_initialize (parser, NULL);
    if (httpp_parse_response (parser, (char*)self->response, self->response_len, self->mount))
    {
        retcode = httpp_getvar (parser, HTTPP_VAR_ERROR_CODE);
        code = atoi(retcode);
        if (code >= 200 && code < 300)
        {
            httpp_destroy (parser);
            self->error = SHOUTERR_SUCCESS;
            return 0;
        }
    }

    httpp_destroy (parser);
    return -1;
}


static int create_xaudiocast_request (shout_t *self)
{
    int ret = -1;

    while (1)
    {
        self->error = SHOUTERR_SOCKET;
        if (add_send_header (self, "SOURCE %s %s\r\n", self->password, self->mount))
            break;

        if (add_send_header (self, "x-audiocast-name: %s\n", self->name != NULL ? self->name : "unnamed"))
            break;
        if (add_send_header (self, "x-audiocast-url: %s\n", self->url != NULL ? self->url : "http://www.icecast.org/"))
            break;
        if (add_send_header (self, "x-audiocast-genre: %s\n", self->genre != NULL ? self->genre : "icecast"))
            break;
        if (add_send_header (self, "x-audiocast-bitrate: %i\n", self->bitrate))
            break;
        if (add_send_header (self, "x-audiocast-public: %i\n", self->public))
            break;
        if (add_send_header (self, "x-audiocast-description: %s\n", self->description != NULL ? self->description : "Broadcasting with the icecast streaming media server!"))
            break;

        if (add_send_header (self, "\n"))
            break;

        self->error = SHOUTERR_SUCCESS;
        ret = 0;
        break;
    }

	return ret;
}


static int login_ok(shout_t *self)
{
    if (shout_read_response (self) < 0)
        return -1;
    self->error = SHOUTERR_NOLOGIN;
    if (!strstr(self->response, "OK"))
        return -1;
    self->error = SHOUTERR_SUCCESS;
    return 0;
}


static int create_icy_request (shout_t *self)
{
    int ret = -1;

    while (1)
    {
        self->error = SHOUTERR_SOCKET;
        /* no decent return code check */
        if (add_send_header (self, "%s\n", self->password))
            break;
        if (add_send_header (self, "icy-name:%s\n", self->name != NULL ? self->name : "unnamed"))
            break;
        if (add_send_header (self, "icy-url:%s\n", self->url != NULL ? self->url : "http://www.icecast.org/"))
            break;
        /* Fields we don't use */
        if (add_send_header (self, "icy-irc:\nicy-aim:\nicy-icq:\n"))
            break;
        if (add_send_header (self, "icy-pub:%i\n", self->public))
            break;
        if (add_send_header (self, "icy-genre:%s\n", self->genre != NULL ? self->genre : "icecast"))
            break;
        if (add_send_header (self, "icy-br:%i\n", self->bitrate))
            break;
        if (add_send_header (self, "\n"))
            break;

        ret = 0;
        self->error = SHOUTERR_SUCCESS;
        break;
    }

	return ret;
}


/* -- public functions -- */

void shout_init(void)
{
    if (_initialized)
        return;
                                                                                                                                                        
    sock_initialize();
    _initialized = 1;
}

void shout_shutdown(void)
{
    if (!_initialized)
        return;

    sock_shutdown();
    _initialized = 0;
}
                                                                                                                                                        

int shout_queue_raw (shout_t *self, const void *data, size_t len)
{
    struct shout_pending_node *node = NULL;

    self->error = SHOUTERR_SOCKET;
    do 
    {
        void *ptr;

        if (self->pending_total + len > self->pending_limit)
            break;
        node = calloc (1, sizeof (*node));
        self->error = SHOUTERR_MALLOC;
        if (node == NULL)
            break;
        ptr = malloc (len);
        if (ptr == NULL)
            break;
        node->data = node->start = ptr;
        memcpy (ptr, data, len);
        node->len = len;
        *self->pending_queue_tail = node;
        self->pending_queue_tail = &node->next;
        self->pending_total += len;
        self->error = SHOUTERR_SUCCESS;
        return 0;
    } while (0);
    if (node)
        free (node);
    return -1;
}



int shout_open(shout_t *self)
{
	/* sanity check */
	if (!self)
		return SHOUTERR_INSANE;

    do 
    {
		self->error = SHOUTERR_INSANE;
        if (!self->host || !self->password || !self->port)
            break;

        self->error = SHOUTERR_CONNECTED;
        if (self->state == SHOUT_CONNECTED)
            break;

        if (self->state == SHOUT_NOCONNECT && self->format_open (self) < 0)
            break;

        if (shout_login (self) < 0)
            break;

        self->starttime = timing_get_time();
        
        self->error = SHOUTERR_SUCCESS;
    } while (0);

	return self->error;
}


const char *shout_version(int *major, int *minor, int *patch)
{
    if (major)
        *major = LIBSHOUT_MAJOR;
    if (minor)
        *minor = LIBSHOUT_MINOR;
    if (patch)
        *patch = LIBSHOUT_MICRO;
    return VERSION;
}

int shout_get_errno(shout_t *self)
{
    return self->error;
}


const char *shout_get_error(shout_t *self)
{
	if (!self)
		return "Invalid shout_t";

	switch (self->error)
    {
        case SHOUTERR_SUCCESS:
            return "No error";
        case SHOUTERR_INSANE:
            return "Nonsensical arguments";
        case SHOUTERR_NOCONNECT:
            return "Couldn't connect";
        case SHOUTERR_NOLOGIN:
            return "Login failed";
        case SHOUTERR_SOCKET:
            return "Socket error";
        case SHOUTERR_MALLOC:
            return "Out of memory";
        case SHOUTERR_CONNECTED:
            return "Still connected";
        case SHOUTERR_UNCONNECTED:
            return "Not connected";
        case SHOUTERR_UNSUPPORTED:
            return "This libshout doesn't support the requested option";
        case SHOUTERR_PENDING:
            return "Operation pending completion";
        case SHOUTERR_PENDING_FULL:
            return "Too much data waiting to be sent";
        case SHOUTERR_BUSY:
            return "Cannot perform task at the moment";
        default:
            return "Unknown error";
	}
}


int shout_get_connected(shout_t *self)
{
    if (self->state != SHOUT_NOCONNECT)
        return SHOUTERR_CONNECTED;
    else
        return SHOUTERR_UNCONNECTED;
}


int shout_set_host(shout_t *self, const char *host)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->host)
		free(self->host);

	self->host = util_strdup(host);
	if (self->host == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_host(shout_t *self)
{
	if (!self)
		return NULL;

	return self->host;
}

int shout_set_port(shout_t *self, unsigned short port)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	self->port = port;

	return self->error = SHOUTERR_SUCCESS;
}

unsigned short shout_get_port(shout_t *self)
{
	if (!self)
		return 0;

	return self->port;
}

int shout_set_password(shout_t *self, const char *password)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->password)
		free (self->password);

	self->password = util_strdup (password);
	if (self->password == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char* shout_get_password(shout_t *self)
{
	if (!self)
		return NULL;

	return self->password;
}

int shout_set_mount(shout_t *self, const char *mount)
{
    size_t len;

	if (!self || !mount)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->mount)
		free(self->mount);

    len = strlen (mount) + 1;
    if (mount[0] != '/')
        len++;

    self->mount = malloc (len);
    if (self->mount == NULL)
        return self->error = SHOUTERR_MALLOC;

    sprintf (self->mount, "%s%s", mount[0] == '/' ? "" : "/", mount);

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_mount(shout_t *self)
{
	if (!self)
		return NULL;

	return self->mount;
}

int shout_set_name(shout_t *self, const char *name)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->name)
		free(self->name);

	self->name = util_strdup (name);
	if (self->name == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_name(shout_t *self)
{
	if (!self)
		return NULL;

	return self->name;
}

int shout_set_url(shout_t *self, const char *url)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->url)
		free(self->url);

	self->url = util_strdup(url);
	if (self->url == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_url(shout_t *self)
{
	if (!self)
		return NULL;

	return self->url;
}

int shout_set_genre(shout_t *self, const char *genre)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->genre)
		free(self->genre);

	self->genre = util_strdup (genre);
	if (self->genre == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_genre(shout_t *self)
{
	if (!self)
		return NULL;

	return self->genre;
}

int shout_set_agent(shout_t *self, const char *agent)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->useragent)
		free(self->useragent);

	self->useragent = util_strdup (agent);
	if (self->useragent == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_agent(shout_t *self)
{
    if (!self)
        return NULL;

    return self->useragent;
}


int shout_set_user(shout_t *self, const char *username)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->user)
		free(self->user);

	self->user = util_strdup (username);
	if (self->user == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_user(shout_t *self)
{
    if (!self)
        return NULL;

    return self->user;
}

int shout_set_description(shout_t *self, const char *description)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	if (self->description)
		free(self->description);

	self->description = util_strdup (description);
	if (self->description == NULL)
		return self->error = SHOUTERR_MALLOC;

	return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_description(shout_t *self)
{
	if (!self)
		return NULL;

	return self->description;
}

int shout_set_dumpfile(shout_t *self, const char *dumpfile)
{
    if (!self)
        return SHOUTERR_INSANE;

    if (self->state != SHOUT_NOCONNECT)
        return SHOUTERR_CONNECTED;

    if (self->dumpfile)
        free(self->dumpfile);

    self->dumpfile = util_strdup (dumpfile);
    if (self->dumpfile == NULL)
        return self->error = SHOUTERR_MALLOC;

    return self->error = SHOUTERR_SUCCESS;
}

const char *shout_get_dumpfile(shout_t *self)
{
    if (!self)
        return NULL;

    return self->dumpfile;
}



/* this really needs looking at, embedding info into one headers can be limiting */
int shout_set_audio_info(shout_t *self, const char *name, const char *value)
{
    return self->error = util_dict_set(self->audio_info, name, value);
}

const char *shout_get_audio_info(shout_t *self, const char *name)
{
    return util_dict_get(self->audio_info, name);
}


int shout_set_public(shout_t *self, unsigned int public)
{
	if (!self || (public != 0 && public != 1))
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

	self->public = public;

	return self->error = SHOUTERR_SUCCESS;
}

unsigned int shout_get_public(shout_t *self)
{
	if (!self)
		return 0;

	return self->public;
}

int shout_set_format(shout_t *self, unsigned int format)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

    switch (format)
    {
        case SHOUT_FORMAT_VORBISPAGE:
        case SHOUT_FORMAT_VORBIS:
            self->format_open = shout_open_vorbis;
            break;
        case SHOUT_FORMAT_MP3:
            self->format_open = shout_open_mp3;
            break;
        default:
            return self->error = SHOUTERR_UNSUPPORTED;
    }

	self->format = format;

	return self->error = SHOUTERR_SUCCESS;
}

unsigned int shout_get_format(shout_t* self)
{
	if (!self)
		return 0;

	return self->format;
}

int shout_set_protocol(shout_t *self, unsigned int protocol)
{
	if (!self)
		return SHOUTERR_INSANE;

	if (self->state != SHOUT_NOCONNECT)
		return self->error = SHOUTERR_CONNECTED;

    switch (protocol)
    {
        case SHOUT_PROTOCOL_HTTP:
            self->get_response = login_http;
            self->create_request = create_http_request;
            break;
        case SHOUT_PROTOCOL_XAUDIOCAST:
            self->get_response = login_ok;
            self->create_request = create_xaudiocast_request;
            break;
        case SHOUT_PROTOCOL_ICY:
            self->get_response = login_ok;
            self->create_request = create_icy_request;
            break;
        default:
            return self->error = SHOUTERR_UNSUPPORTED;
    }

	self->protocol = protocol;

	return self->error = SHOUTERR_SUCCESS;
}

unsigned int shout_get_protocol(shout_t *self)
{
	if (!self)
		return 0;

	return self->protocol;
}

int shout_connection_ready (shout_t *self)
{
    if (self)
        return self->state == SHOUT_READY;
    return 0;
}


shout_metadata_t *shout_metadata_new(void)
{
    return util_dict_new();
}

void shout_metadata_free(shout_metadata_t *self)
{
    if (!self)
        return;
                                                                                                                                                       
    util_dict_free(self);
}
                                                                                                                                                       
int shout_metadata_add(shout_metadata_t *self, const char *name, const char *value)
{
    if (!self || !name)
        return SHOUTERR_INSANE;
    return util_dict_set(self, name, value);
}



int shout_set_metadata(shout_t *self, shout_metadata_t *metadata)
{
    sock_t socket;
    int ret;
    char *encvalue;

    if (!self)
        return SHOUTERR_INSANE;

    if (metadata == NULL)
        return self->error = SHOUTERR_INSANE;

    if (self->state != SHOUT_READY)
        return self->error = SHOUTERR_UNCONNECTED;

    encvalue = util_dict_urlencode(metadata, '&');
    if (encvalue == NULL)
        return self->error = SHOUTERR_MALLOC;

    ret = SHOUTERR_NOCONNECT;
    if ((socket = sock_connect(self->host, self->port)) >= 0)
    {
        int rv = 1;
        if (self->protocol == SHOUT_PROTOCOL_ICY)
            rv = sock_write(socket, "GET /admin.cgi?mode=updinfo&pass=%s&%s HTTP/1.0\r\nUser-Agent: %s (Mozilla compatible)\r\n\r\n",
                    self->password, encvalue, shout_get_agent(self));
        else if (self->protocol == SHOUT_PROTOCOL_HTTP)
        {
            char *auth = http_basic_authorization(self);

            rv = sock_write(socket, "GET /admin/metadata?mode=updinfo&mount=%s&%s HTTP/1.0\r\nUser-Agent: %s\r\n%s\r\n",
                    self->mount, encvalue, shout_get_agent(self), auth ? auth : "");
        }
        if (rv)
            ret = SHOUTERR_SUCCESS;
        else
            ret = SHOUTERR_SOCKET;
        sock_close(socket);
    }

    free (encvalue);

    return self->error = ret;
}


int shout_send_raw (shout_t *self, const void *data, size_t len)
{
    int written = 0;
    const char *ptr = data;

    if (self->pending_queue)
        shout_send_pending (self);

    if (self->pending_queue == NULL)
    {
        written = shout_try_write (self, data, len);
        if (written == (int)len || written < 0)
            return written;
    }
    if (shout_queue_raw (self, ptr+written, len-written) < 0)
        return -1;
    return (int)len;
}



void shout_sync(shout_t *self)
{
    int64_t sleep;

    if (!self)
        return;

    if (self->senttime == 0)
        return;

    sleep = self->senttime/1000 - (timing_get_time() - self->starttime);

    // printf ("senttime is %lld, sleep is %lld\n", self->senttime, sleep);
    if (sleep > 0)
        timing_sleep((uint64_t)sleep);
}

int shout_delay(shout_t *self)
{
    if (!self)
        return 0;

    if (self->senttime == 0)
        return 0;

    return self->senttime / 1000 - (timing_get_time() - self->starttime);
}


int shout_send(shout_t *self, const void *data, size_t len)
{
    if (self)
    {
        if (self->state != SHOUT_READY)
            return self->error = SHOUTERR_UNCONNECTED;

        if (self->send)
            return self->send(self, data, len);

        self->error = SHOUTERR_INSANE;
    }
    return SHOUTERR_INSANE;
}


shout_t *shout_new(void)
{
	shout_t *self;

    shout_init();  /* initialise lib is not already */

	self = (shout_t *)calloc(1, sizeof(shout_t));
	while (self) 
    {

        if (shout_set_host (self, LIBSHOUT_DEFAULT_HOST))
            break;

        self->port = LIBSHOUT_DEFAULT_PORT;
        shout_set_format (self,  LIBSHOUT_DEFAULT_FORMAT);
        shout_set_protocol (self, LIBSHOUT_DEFAULT_PROTOCOL);
        
        if (shout_set_user (self, LIBSHOUT_DEFAULT_USER) < SHOUTERR_SUCCESS)
            break;
        if (shout_set_agent (self, LIBSHOUT_DEFAULT_USERAGENT) < SHOUTERR_SUCCESS)
            break;

        if ((self->audio_info = util_dict_new()) == NULL)
            break;
        
        self->send_header = malloc (MAX_HTTP_SENT);
        if (self->send_header == NULL)
            break;
        self->send_header_offset = 0;

        self->response = malloc (MAX_HTTP_RESPONSE);
        if (self->response == NULL)
            break;
        self->read_timeout = 10;   /* default 10 seconds */
        self->pending_total = 0;
        self->pending_limit = 65535;  /* don't queue more than this */
        self->pending_queue_tail = &self->pending_queue;
        self->response_len = 0;

        return self;
    }
    shout_free(self);

	return NULL;
}


void shout_set_read_timeout (shout_t *self, unsigned timeout)
{
    if (self)
        self->read_timeout = timeout;
}


void shout_set_nonblocking (shout_t *self, int nonblock)
{
     self->nonblocking = nonblock;
     if (sock_valid_socket(self->socket))
         sock_set_blocking (self->socket, nonblock?SOCK_NONBLOCK:SOCK_BLOCK);
}


int shout_close(shout_t *self)
{
    if (!self)
        return SHOUTERR_INSANE;

    self->starttime = 0;
    if (self->send_header)
    {
        self->send_header_offset = 0;
        self->send_header_size = 0;
    }
    if (self->response)
    {
        self->response_len = 0;
    }

    /* send what we can if any pending */
    shout_send_pending (self);
    /* do memory cleanup */
    shout_free_pending (self);

    if (self->close)
        self->close(self);

    if (self->socket > -1)
        sock_close(self->socket);
    self->socket = -1;
    self->pending_connection = 0;

    if (self->state == SHOUT_NOCONNECT)
        self->error = SHOUTERR_UNCONNECTED;
    else
        self->error = SHOUTERR_SUCCESS;

    self->state = SHOUT_NOCONNECT;
	return self->error;
}


void shout_free(shout_t *self)
{
	if (!self) return;

	if (self->host) free(self->host);
	if (self->password) free(self->password);
	if (self->mount) free(self->mount);
	if (self->name) free(self->name);
	if (self->url) free(self->url);
	if (self->genre) free(self->genre);
	if (self->description) free(self->description);
	if (self->user) free(self->user);
    if (self->useragent) free(self->useragent);
    if (self->audio_info) util_dict_free (self->audio_info);
    if (self->response) free (self->response);
    if (self->send_header) free (self->send_header);

    shout_free_pending (self);
	free(self);
}

