#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "iceadmin.h"
#include "iceadmin_internal.h"
#include "error.h"
#include "xalloc.h"

/* -1 ~= illegal, 1 ~= take as-is (safe), 0 ~= needs encoding */
const int _url_safe_chars[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 00-0F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 10-1F */
     0,  1,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  0,  1,  1,  0, /* 20-2F */
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0, /* 30-3F */
     0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* 40-4F */
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  1, /* 50-5F */
     0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* 60-6F */
     1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0, -1, /* 70-7F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 80-8F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 90-9F */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* A0-AF */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* B0-BF */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* C0-CF */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* D0-DF */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /* E0-EF */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  /* F0-FF */
};

const unsigned char _hex_chars[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

const unsigned char _base64_chars[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};
const unsigned char _base64_pad = '=';

char *
_iceadmin_url_encode(const char *str)
{
	char		*enc_str, *cp;
	const char	*ccp;
	size_t		 size;
	unsigned int	 safe_chars, encode_chars;

	if (str == NULL)
		return (NULL);

	safe_chars = encode_chars = 0;

	for (ccp = str; *ccp != '\0'; ccp++) {
		switch (_url_safe_chars[(unsigned int)*ccp]) {
		case -1:
			return (NULL);
		case 0:
			encode_chars++;
			break;
		case 1:
			safe_chars++;
			break;
		default:
			return (NULL);
		}
	}

	size = 3 * encode_chars + safe_chars + 1;
	enc_str = xcalloc(size, sizeof(char));

	for (ccp = str, cp = enc_str; *ccp != '\0'; ccp++, cp++) {
		if (_url_safe_chars[(unsigned int)*ccp]) {
			*cp = *ccp;
			continue;
		}

		*cp++ = '%';
		*cp++ = _hex_chars[(*ccp >> 4) & 0xF];
		*cp = _hex_chars[*ccp & 0xF];
	}

	return (enc_str);
}

#define ISXDIGIT(x)	isxdigit((int)((unsigned char)x))
char *
_iceadmin_url_decode(const char *str)
{
	char		*dec_str, *cp;
	const char	*ccp;
	size_t		 str_size;

	str_size = strlen(str) + 1;
	dec_str = xcalloc(str_size, sizeof(char));
	for (ccp = str, cp = dec_str; *ccp != '\0'; ccp++) {
		if (*ccp == '%' && ISXDIGIT(ccp[1]) && ISXDIGIT(ccp[2])) {
			char	hexstr[3];
			long	hex;

			hexstr[0] = ccp[1];
			hexstr[1] = ccp[2];
			hexstr[2] = '\0';
			hex = strtol(hexstr, NULL, 16);
			if (_url_safe_chars[(unsigned int)hex] != -1)
				*cp++ = (unsigned char)hex;
			ccp += 2;
		} else
			*cp++ = *ccp;
	}

	return (dec_str);
}

char *
_iceadmin_auth_basic_string(const char *user, const char *pass)
{
	size_t	 siz1, siz2;
	char	*buf1, *buf2;

	if (user == NULL || pass == NULL)
		return (xstrdup(""));

	siz1 = strlen(user) + 1 + strlen(pass) + 1;
	buf1 = xcalloc(siz1, sizeof(char));
	snprintf(buf1, siz1, "%s:%s", user, pass);
	if (_iceadmin_base64_encode(buf1, siz1 - 1, &buf2, &siz2) != ICEADMIN_SUCCESS) {
		xfree(buf1);
		return (xstrdup(""));
	}
	xfree(buf1);

	siz1 = 24 + siz2 + 1;
	buf1 = xcalloc(siz1, sizeof(char));
	snprintf(buf1, siz1, "Authorization: Basic %s\r\n", buf2);
	xfree(buf2);

	return (buf1);
}

/*
 * Based on b64_ntop() by IBM and ISC, from
 * $OpenBSD: src/lib/libc/net/base64.c,v 1.5 2006/10/21 09:55:03 otto Exp $
 * License is in b64_ntop.txt
 */
int
_iceadmin_base64_encode(const char *in, size_t inputlen, char **out_p, size_t *outputlen)
{
	size_t		 datalen;
	unsigned char	 input[3];
	unsigned char	 output[4];
	char		*out;
	size_t		 bufsize;

	bufsize = (inputlen * 4 / 3) + 4;
	out = xcalloc(bufsize, sizeof(char));

	datalen = 0;
	while (inputlen > 2) {
		input[0] = *in++;
		input[1] = *in++;
		input[2] = *in++;
		inputlen -= 3;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = input[2] & 0x3f;

		if (datalen + 4 > bufsize) {
			xfree(out);
			return (ICEADMIN_INSANE);
		}

		out[datalen++] = _base64_chars[output[0]];
		out[datalen++] = _base64_chars[output[1]];
		out[datalen++] = _base64_chars[output[2]];
		out[datalen++] = _base64_chars[output[3]];
	}

	if (inputlen != 0) {
		unsigned int	i;

		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < inputlen; i++)
			input[i] = *in++;

		output[0] = input[0] >> 2;
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		if (datalen + 4 > bufsize) {
			xfree(out);
			return (ICEADMIN_INSANE);
		}

		out[datalen++] = _base64_chars[output[0]];
		out[datalen++] = _base64_chars[output[1]];
		if (inputlen == 1)
			out[datalen++] = _base64_pad;
		else
			out[datalen++] = _base64_chars[output[2]];
		out[datalen++] = _base64_pad;
	}

	if (datalen >= bufsize) {
		xfree(out);
		return (ICEADMIN_INSANE);
	}

	out[datalen] = '\0';
	*out_p = xrealloc(out, datalen + 1, sizeof(char));
	if (outputlen != NULL)
		*outputlen = datalen;

	return (ICEADMIN_SUCCESS);
}

int
_iceadmin_read_http_reply(struct iceadmin *iceadmin, int sock, unsigned int *httpret)
{
	char		static_buf[BUFSIZ], *bp, ch;
	long		rret;
	unsigned int	i;

	static_buf[0] = '\0';
	bp = static_buf;
	i = 0;
	while ((rret = (long)read(sock, &ch, sizeof(char))) != -1 && rret != 0) {
		if (++i >= sizeof(static_buf) - 1) {
			_set_error(iceadmin, 0, 0, "[%s}:%s: (Non-)HTTP reply too large",
				   iceadmin->hostname, iceadmin->service);
			return (ICEADMIN_ERROR);
		}
		if ((*bp++ = ch) == '\n') {
			*bp = '\0';
			break;
		}
	}
	if (rret == -1) {
		_set_error(iceadmin, 0, errno, "read: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	if (strncmp(static_buf, "HTTP/1.", 7) != 0) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: No HTTP reply from server",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	if (sscanf(static_buf, "HTTP/%*s %u %*s", httpret) != 1) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: HTTP parse error",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	if (*httpret != 200) {
		switch (*httpret) {
		case 400:
			/*
			 * Context sensitive, but set a sane default. Caller
			 * should make this more specific.
			 */
			_set_error(iceadmin, 0, 0, "[%s]:%s: 400 - Bad request; not an Icecast server?",
				   iceadmin->hostname, iceadmin->service);
			break;
		case 401:
			_set_error(iceadmin, 0, 0, "[%s]:%s: 401 - Authentication for user '%s' failed",
				   iceadmin->hostname, iceadmin->service,
				   iceadmin->username);
			break;
		case 403:
			_set_error(iceadmin, 0, 0, "[%s]:%s: 403 - Access denied; not an Icecast server?",
				   iceadmin->hostname, iceadmin->service);
			break;
		case 404:
			_set_error(iceadmin, 0, 0, "[%s]:%s: 404 - Not found; not an Icecast server?",
				   iceadmin->hostname, iceadmin->service);
			break;
		default:
			_set_error(iceadmin, 0, 0, "[%s]:%s: %3u - Unsupported HTTP reply",
				   iceadmin->hostname, iceadmin->service, *httpret);
			break;
		}
		return (ICEADMIN_ERROR);
	}

	return (ICEADMIN_SUCCESS);
}

/*
 * Based on FreeBSD's strnstr():
 * $FreeBSD: src/lib/libc/string/strnstr.c,v 1.4 2007/01/09 00:28:12 imp Exp $
 * License is in strnstr.txt
 */
char *
_iceadmin_strnstr(const char *s, const char *find, size_t slen, size_t *pos_p)
{
	char c, sc;
	size_t len, pos;

	pos = 0;
	if ((c = *find++) != '\0') {
		len = strlen(find);
		do {
			do {
				if (slen-- < 1 || (sc = *s++) == '\0')
					return (NULL);
				else
					pos++;
			} while (sc != c);
			if (len > slen)
				return (NULL);
		} while (strncmp(s, find, len) != 0);
		s--;
		pos--;
	}

	if (pos_p != NULL)
		*pos_p = pos;

	return ((char *)s);
}

int
_iceadmin_read_xml_data(struct iceadmin *iceadmin, int sock, char **buf_p, size_t *bufsize_p)
{
	char	*buf, *bp, static_buf[BUFSIZ];
	size_t	 bufsize, pos;
	long	 rret;

	/*
	 * Fill the buffer with a chunk of data. We're looking for the start
	 * of XML data. So, if it's not here, we can be sure that we received
	 * junk.
	 */
	if ((rret = (long)read(sock, static_buf, sizeof(static_buf))) == -1) {
		_set_error(iceadmin, 0, errno, "read: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}
	if ((bp = _iceadmin_strnstr(static_buf, "<?xml version=", sizeof(static_buf), &pos)) == NULL) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: No XML data in reply from server",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	bufsize = rret - pos + 1;
	buf = xmalloc(bufsize);
	memcpy(buf, bp, rret - pos);
	pos = rret - pos;

	while ((rret = (long)read(sock, static_buf, sizeof(static_buf))) != -1 && rret != 0) {
		bufsize += rret;
		buf = xrealloc(buf, bufsize, sizeof(char));
		memcpy(&buf[pos], static_buf, rret);
		pos += rret;
	}
	if (rret == -1) {
		_set_error(iceadmin, 0, errno, "read: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		if (buf != NULL)
			xfree(buf);
		return (ICEADMIN_ERROR);
	}
	buf[pos] = '\0';
	if (pos > 0 && buf[pos - 1] == '\n')
		buf[pos - 1] = '\0';

	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	*buf_p = buf;

	return (ICEADMIN_SUCCESS);
}

int
_iceadmin_connect(struct iceadmin *iceadmin, int *sock)
{
	if ((*sock = socket(iceadmin->domain, iceadmin->type,
			   iceadmin->protocol)) == -1) {
		_set_error(iceadmin, 0, errno, "socket");
		return (ICEADMIN_ERROR);
	}

	if (connect(*sock, iceadmin->address, iceadmin->addrsize) == -1) {
		_set_error(iceadmin, 0, errno, "connect: [%s]:%s",
			   iceadmin->hostname, iceadmin->service);
		close(*sock);
		return (ICEADMIN_ERROR);
	}

	return (ICEADMIN_SUCCESS);
}

int
_iceadmin_format_xmltree(struct iceadmin *iceadmin, char **buf_p,
			 size_t *bufsize_p, xmlDocPtr doc)
{
	xmlNodePtr	  node;
	char		 *buf;
	size_t		  bufsize;
	char		**strbuf;
	size_t		  strbuf_size;

	if ((node = xmlDocGetRootElement(doc)) == NULL) {
		_set_error(iceadmin, 0, 0, "[%s]:%s: XML parse error (empty document)",
			   iceadmin->hostname, iceadmin->service);
		return (ICEADMIN_ERROR);
	}

	strbuf_size = 32;	/* This is also the maximum recursion depth. */
	strbuf = xcalloc(strbuf_size, sizeof(char *));
	bufsize = 1;
	buf = xcalloc(bufsize, sizeof(char));
	_iceadmin_walk_xmltree(&buf, &bufsize, doc, node, strbuf, strbuf_size, 0);
	xfree(strbuf);

	*buf_p = buf;
	if (bufsize_p != NULL)
		*bufsize_p = bufsize;

	return (ICEADMIN_SUCCESS);
}

void
_iceadmin_walk_xmltree(char **outbuf_p, size_t *outbuf_size, xmlDocPtr doc,
		       xmlNodePtr node, char **strbuf,
		       size_t strbuf_size, unsigned int depth)
{
	xmlChar 	 *content;
	xmlAttrPtr	  attr;
	char		 *attr_buf;
	size_t		  attr_bufsize;

	if (depth >= strbuf_size - 1) {
		/* Maximum recursion depth reached */
		return;
	}

	attr_buf = NULL;
	attr_bufsize = 0;
	if (node->properties != NULL) {
		attr_bufsize = 2;
		attr_buf = xmalloc(attr_bufsize);
		attr_buf[0] = '[';
		attr_buf[1] = '\0';
		for (attr = node->properties; attr != NULL; attr = attr->next) {
			attr_bufsize += strlen((const char *)attr->name);
			attr_buf = xrealloc(attr_buf, attr_bufsize, sizeof(char));
			strlcat(attr_buf, (const char *)attr->name, attr_bufsize);

			if (attr->children != NULL && attr->children->content != NULL) {
				attr_bufsize += strlen((const char *)attr->children->content) + 3;
				attr_buf = xrealloc(attr_buf, attr_bufsize,
						    sizeof(char));
				strlcat(attr_buf, "=\"", attr_bufsize);
				strlcat(attr_buf, (const char *)attr->children->content,
					attr_bufsize);
				strlcat(attr_buf, "\"", attr_bufsize);
			}

			if (attr->next != NULL) {
				attr_bufsize++;
				attr_buf = xrealloc(attr_buf, attr_bufsize,
						    sizeof(char));
				strlcat(attr_buf, ";", attr_bufsize);
			}
		}
		attr_bufsize++;
		attr_buf = xrealloc(attr_buf, attr_bufsize, sizeof(char));
		strlcat(attr_buf, "]", attr_bufsize);
	}

	if (attr_buf != NULL) {
		size_t	siz;

		siz = strlen((const char *)node->name) + attr_bufsize;
		strbuf[depth] = xcalloc(siz, sizeof(char));
		snprintf(strbuf[depth], siz, "%s%s", (const char *)node->name, attr_buf);
		xfree(attr_buf);
	} else
		strbuf[depth] = xstrdup((const char *)node->name);
	strbuf[depth + 1] = NULL;

	for (node = node->xmlChildrenNode; node != NULL; node = node->next) {
		if ((content = xmlNodeListGetString(doc, node, 1)) != NULL) {
			unsigned int	i;
			char		*str;

			for (i = 0; strbuf[i] != NULL; i++) {
				*outbuf_size += strlen(strbuf[i]);
				*outbuf_p = xrealloc(*outbuf_p, *outbuf_size, sizeof(char));
				strlcat(*outbuf_p, strbuf[i], *outbuf_size);
				if (strbuf[i + 1] != NULL) {
					*outbuf_size += 1;
					*outbuf_p = xrealloc(*outbuf_p, *outbuf_size, sizeof(char));
					strlcat(*outbuf_p, ".", *outbuf_size);
				}
			}

			str = _iceadmin_url_decode((const char *)content);
			xmlFree(content);
			*outbuf_size += strlen(str) + 6;
			*outbuf_p = xrealloc(*outbuf_p, *outbuf_size, sizeof(char));
			strlcat(*outbuf_p, " = \"", *outbuf_size);
			strlcat(*outbuf_p, str, *outbuf_size);
			xfree(str);
			strlcat(*outbuf_p, "\"\n", *outbuf_size);
		} else {
			_iceadmin_walk_xmltree(outbuf_p, outbuf_size, doc,
					       node, strbuf, strbuf_size,
					       depth + 1);
		}
	}

	xfree(strbuf[depth]);
}

int
_iceadmin_get_xml_reply(char *buf, size_t bufsize, char *retbuf, size_t retbuf_size)
{
	xmlDocPtr	doc;
	xmlNodePtr	node;
	int		ret;

	if ((doc = xmlParseMemory(buf, bufsize)) == NULL)
		return (-1);

	if ((node = xmlDocGetRootElement(doc)) == NULL) {
		xmlFreeDoc(doc);
		return (-1);
	}

	ret = -1;
	for (node = node->xmlChildrenNode; node != NULL; node = node->next) {
		if (xmlStrcmp(node->name, BAD_CAST "message") == 0) {
			xmlChar *content;
			char	*str;

			content = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
			if (content == NULL)
				continue;
			str = _iceadmin_url_decode((const char *)content);
			xmlFree(content);
			snprintf(retbuf, retbuf_size, "%s", str);
			xfree(str);
		} else if (xmlStrcmp(node->name, BAD_CAST "return") == 0) {
			xmlChar *content;

			content = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
			if (content == NULL)
				continue;
			if (content[0] == '0')
				ret = 0;
			else if (content[0] == '1')
				ret = 1;
			xmlFree(content);
		}
	}

	xmlFreeDoc(doc);

	return (ret);
}
