/* cfgparse.c
 * - setup file reading code, plus default settings.
 *
 * Copyright (c) 2002-4 Karl Heyes <karl@xiph.org>
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
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
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

#ifdef HAVE_STRINGS_H
 #include <strings.h>
#endif

#include <ogg/ogg.h>

/* these might need tweaking for other systems */
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "thread/thread.h"

#include "cfgparse.h"
#include "thread/thread.h"
#include "inputmodule.h"
#include "encode.h"
#include "runner.h"
#include "om_file.h"
#include "om_shout.h"

#define DEFAULT_PLAYLIST_MODULE "playlist"
#define DEFAULT_STREAM_NAME "unnamed ices stream"
#define DEFAULT_STREAM_GENRE "ices unset"
#define DEFAULT_STREAM_DESCRIPTION "no description set"
#define DEFAULT_LOGPATH "/tmp"
#define DEFAULT_LOGFILE "ices.log"
#define DEFAULT_LOGLEVEL 1
#define DEFAULT_LOGSIZE 2048
#define DEFAULT_LOG_STDERR 1
#define DEFAULT_BACKGROUND 0

#ifdef DEBUG_CFG
#define dprintf(...)      printf(__VA_ARGS__)
#else
#define dprintf(...)
#endif

/* this is the global config variable */
config_t *ices_config;

#ifndef HAVE_STRNDUP
char *strndup(const char *str, size_t len)
{
    char *dup= malloc (len+1);
    if (dup == NULL) abort();
    strncpy (dup, str, len);
    dup[len]= '\0';
    return dup;
}
#endif


int get_xml_float (xmlNodePtr node, void *x)
{
    char *tmp = (char *)xmlNodeListGetString (node->doc, node->xmlChildrenNode, 1);
    if (tmp == NULL)
        return -1;
    *(float*)x = atof (tmp);
    xmlFree(tmp);
    return 0;
}


int get_xml_bool (xmlNodePtr node, void *x)
{
    char *str = (char *)xmlNodeListGetString (node->doc, node->xmlChildrenNode, 1);
    if (str == NULL)
        return -1;
    if (strcasecmp (str, "true") == 0)
        *(int*)x = 1;
    else
        if (strcasecmp (str, "yes") == 0)
            *(int*)x = 1;
        else
            *(int*)x = strtol (str, NULL, 0)==0 ? 0 : 1;
    xmlFree (str);
    return 0;
}


int get_xml_int (xmlNodePtr node, void *x)
{
    char *tmp = (char *)xmlNodeListGetString (node->doc, node->xmlChildrenNode, 1);
    if (tmp == NULL)
        return -1;
    *(int*)x = strtol(tmp, NULL, 0);
    xmlFree(tmp);
    return 0;
}


int get_xml_string (xmlNodePtr node, void *x)
{
    char *str = (char *)xmlNodeListGetString (node->doc, node->xmlChildrenNode, 1);
    char *p = *(char**)x;
    if (str == NULL)
        return -1;
    if (p)
    {
        dprintf ("freeing \"%s\" (%p) alloc \"%s\"\n", p, p, str);
        xmlFree (p);
    }
    *(char **)x = str;
    return 0;
}


int parse_xml_tags (const char *id, xmlNodePtr node, const struct cfg_tag *args)
{
    int ret = 0;

    for (; node != NULL && ret == 0; node = node->next)
    {
        const struct cfg_tag *argp;

        if (xmlIsBlankNode (node) || strcmp ((char*)node->name, "comment") == 0 ||
                strcmp ((char*)node->name, "text") == 0)
            continue;
        argp = args;
        dprintf ("Checking \"%s\"\n", node->name);
        while (argp->name)
        {
            dprintf ("  against \"%s\"\n", argp->name);
            if (strcmp ((const char*)node->name, argp->name) == 0)
            {
                ret = argp->retrieve (node, argp->storage);
                break;
            }
            argp++;
        }
        if (argp->name == NULL)
            fprintf (stderr, "%s:  unknown element \"%s\" parsing \"%s\"\n", ices_config->cfgfile, node->name, id);
    }
    dprintf (" Ret is %d\n", ret);
    return ret;
}




static void _free_params(module_param_t *param)
{
	while (param != NULL)
	{
        module_param_t *next;

		if (param->name) xmlFree(param->name);
		if (param->value) xmlFree(param->value);
		next = param->next;
		free(param);
		param = next;
    }
}


static input_module_t *_free_input_module (input_module_t *mod)
{
    input_module_t *next = mod->next;

    if (mod->module != DEFAULT_PLAYLIST_MODULE) 
        xmlFree (mod->module);

    if (mod->module_params)
        _free_params(mod->module_params);

    free (mod);
    return next;
}



static int _parse_input_param (xmlNodePtr node, void *arg)
{
    module_param_t *param, *p;
    input_module_t *inp = arg;

    param = (module_param_t *)calloc(1, sizeof(module_param_t));
    if (param)
    {
        if (get_xml_param_name (node, "name", &param->name) < 0)
        {
            free (param);
            return -1;
        }
        if (get_xml_string (node, &param->value) < 0)
        {
            xmlFree (param->name);
            free (param);
            return -1;
        }
        if (inp->module_params == NULL) 
            inp->module_params = param;
        else 
        {
            p = inp->module_params;
            while (p->next != NULL) p = p->next;
            p->next = param;
        }
        return 0;
    }
    return -1;
}


static int _known_module (xmlNodePtr node, void *arg)
{
    int current_module = 0, ret = -1;
    input_module_t *inp = arg;

    if (get_xml_string (node, &inp->module) < 0)
        return -1;

    while (modules[current_module].name)
    {
        if(!strcmp(inp->module, modules[current_module].name))
        {
            inp->initialise_module = modules[current_module].initialise;
            inp->open_module = modules[current_module].open;
            inp->close_module = modules[current_module].close;
            inp->shutdown_module = modules[current_module].shutdown;
            ret = 0;
            break;
        }
        current_module++;
    }
    return ret;
}

static int _parse_input(xmlNodePtr node, void *arg)
{
    config_t *config = arg;
    int save = 1;
    input_module_t *mod = calloc (1, sizeof (input_module_t));
    static unsigned input_id = 0;

    while (mod)
    {
        struct cfg_tag input_tag[] =
        {
            { "module",     _known_module,      mod },
            { "param",      _parse_input_param, mod },
            { "buffers",    get_xml_int,        &mod->buffer_count },
            { "prealloc",   get_xml_int,        &mod->prealloc_count },
            { "save",       get_xml_int,        &save },
            { NULL, NULL, NULL }
        };
        if (parse_xml_tags ("input", node->xmlChildrenNode, input_tag))
            break;
        mod->id = ++input_id;
        /* mod->save = save; */
        if (config->inputs == NULL)
            config->inputs = mod;
        else
        {
            input_module_t *i = config->inputs;
            while (i->next != NULL) i = i->next;
            i->next = mod;
        }
        return 0;
    }
    if (mod)  free (mod);
    return -1;
}




static int _parse_stream (xmlNodePtr node, void *arg)
{
    config_t *config = arg;
    struct cfg_tag stream_tag[] =
    {
        { "name",           get_xml_string,     &config->stream_name },
        { "genre",          get_xml_string,     &config->stream_genre },
        { "description",    get_xml_string,     &config->stream_description },
        { "url",            get_xml_string,     &config->stream_url },
        { "once",           get_xml_bool,       &config->input_once_thru },
        { "input",          _parse_input,       config },
        { "runner",         parse_runner,       config },
        { NULL, NULL, NULL }
    };

    return parse_xml_tags ("stream", node->xmlChildrenNode, stream_tag);
}


static int _parse_root (xmlNodePtr node, void *arg)
{
    config_t *config = arg;
    char *user = NULL;

    if (config && node && strcmp((char*)node->name, "ices") == 0)
    {
        int realtime = 1;
        struct cfg_tag ices_tag[] =
        {
            { "background", get_xml_bool,       &config->background },
            { "realtime",   get_xml_bool,       &realtime },
            { "user",       get_xml_string,     &user },
            { "logpath",    get_xml_string,     &config->logpath },
            { "logfile",    get_xml_string,     &config->logfile },
            { "loglevel",   get_xml_int,        &config->loglevel },
            { "logsize",    get_xml_int,        &config->logsize },
            { "consolelog", get_xml_bool,       &config->log_stderr },
            { "pidfile",    get_xml_string,     &config->pidfile },
            { "stream",     _parse_stream,      config },
            { NULL, NULL, NULL }
        };
        if (parse_xml_tags ("ices", node->xmlChildrenNode, ices_tag))
            return -1;

        config->realtime = realtime;
        config->user = user;
        return 0;
    }
    return -1;
}

static config_t *allocate_config(void)
{
	config_t *c = (config_t *)calloc(1, sizeof(config_t));

    if (c == NULL)
        return NULL;
	c->background = DEFAULT_BACKGROUND;
	c->logpath = xmlStrdup (DEFAULT_LOGPATH);
	c->logfile = xmlStrdup (DEFAULT_LOGFILE);
	c->logsize = DEFAULT_LOGSIZE;
	c->loglevel = DEFAULT_LOGLEVEL;
    c->log_stderr = DEFAULT_LOG_STDERR;
	c->stream_name = xmlStrdup (DEFAULT_STREAM_NAME);
	c->stream_genre = xmlStrdup (DEFAULT_STREAM_GENRE);
	c->stream_description = xmlStrdup (DEFAULT_STREAM_DESCRIPTION);
    dprintf ("name initially at  %p \"%s\"\n", c->stream_name, c->stream_name);
    dprintf ("desc initially at  %p \"%s\"\n", c->stream_description, c->stream_description);
    dprintf ("genre initially at %p \"%s\"\n", c->stream_genre, c->stream_genre);

    return c;
}

void config_initialize(void)
{
	srand(time(NULL));
}

void config_shutdown(void)
{
    struct runner *run;
    input_module_t *mod;

	if (ices_config == NULL) return;

    mod = ices_config->inputs;
    while (mod)
    {
        mod->shutdown_module (mod);
        mod = _free_input_module (mod);
    }

    if (ices_config->logpath != DEFAULT_LOGPATH)
        xmlFree (ices_config->logpath);

    if (ices_config->logfile != DEFAULT_LOGFILE)
        xmlFree (ices_config->logfile); 

    if (ices_config->stream_name != DEFAULT_STREAM_NAME) 
        xmlFree (ices_config->stream_name);

    if (ices_config->stream_genre !=  DEFAULT_STREAM_GENRE) 
        xmlFree (ices_config->stream_genre);

    if (ices_config->stream_description != DEFAULT_STREAM_DESCRIPTION) 
        xmlFree (ices_config->stream_description);

    if (ices_config->stream_url) 
        xmlFree (ices_config->stream_url);

    if (ices_config->user)
        xmlFree (ices_config->user);

    run = ices_config->runners;
    while (run)
        run = config_free_runner (run);
    
	free(ices_config);
	ices_config = NULL;
}

int config_read(const char *fn)
{
    xmlDocPtr doc = NULL;
    int ret = 0;
    uid_t euid = geteuid();

#ifdef _POSIX_SAVED_IDS
    if (seteuid (getuid()) < 0)
#else
    uid_t ruid = getuid();
    if (setreuid (euid, ruid) < 0)
#endif
    {
        fprintf (stderr, "failed to drop priviledges for reading config file\n");
    }
    xmlInitParser ();
    doc = xmlParseFile(fn);
    xmlCleanupParser ();

    ices_config = allocate_config();
    ices_config->cfgfile = fn;
    if (_parse_root (xmlDocGetRootElement (doc), ices_config))
        ret = -1;

    xmlFreeDoc (doc);
#ifdef _POSIX_SAVED_IDS
    if (seteuid (euid) < 0)
#else
    if (setreuid (ruid, euid) < 0)
#endif
        fprintf (stderr, "failed to reset privilidges after reading config file\n");

    return ret;
}


