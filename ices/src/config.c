/* config.c
 * - config file reading code, plus default settings.
 *
 * $Id: config.c,v 1.6.2.1 2002/02/07 09:11:10 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* these might need tweaking for other systems */
#include <xmlmemory.h>
#include <parser.h>

#include "thread.h"

#include "config.h"
#include "stream.h"
#include "thread/thread.h"
#include "registry.h"

#define DEFAULT_BACKGROUND 0
#define DEFAULT_LOGPATH "/tmp"
#define DEFAULT_LOGFILE "ices.log"
#define DEFAULT_LOGLEVEL 1
#define DEFAULT_LOG_STDERR 1

#define DEFAULT_MAX_QUEUE_LENGTH 100 /* Make it _BIG_ by default */

/* helper macros so we don't have to write the same
** stupid code over and over
*/
#define SET_STRING(x) \
	do {\
		if (x) free(x);\
		(x) = (char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);\
	} while (0) 

#define SET_INT(x) \
	do {\
		char *tmp = (char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);\
		(x) = atoi(tmp);\
		if (tmp) free(tmp);\
	} while (0)

#define SET_FLOAT(x) \
	do {\
		char *tmp = (char *)xmlNodeListGetString(doc, node->xmlChildrenNode, 1);\
		(x) = atof(tmp);\
		if (tmp) free(tmp);\
	} while (0)

#define SET_PARM_STRING(p,x) \
        do {\
                if (x) free(x);\
                (x) = (char *)xmlGetProp(node, p);\
	} while (0)


/* this is the global config variable */
config_t *ices_config;

void config_free_instance(instance_t *instance)
{
	if (instance->queue) 
	{
		thread_mutex_destroy(&instance->queue->lock);
		free(instance->queue);
	}
    free(instance);
}

void config_free_params(module_param_t *param)
{
	module_param_t *next;
	next = NULL;
	do 
	{
		if (param->name) free(param->name);
		if (param->value) free(param->value);
		next = param->next;
		free(param);
		
		param = next;
	} while (next != NULL);
}

static process_chain_element *_parse_module(xmlDocPtr doc, xmlNodePtr node,
        process_chain_element *chain_start)
{
    process_chain_element *module = calloc(1, sizeof(process_chain_element));
    char *modulename=NULL;
    int i = 0, foundmodule=0;
    module_param_t *param, *params=NULL;
    
    if(node == NULL || xmlIsBlankNode(node)) {
        fprintf(stderr, "Null or empty node in config read\n");
        return NULL;
    }

    SET_PARM_STRING("name", modulename);
    while(registered_modules[i].open) {
        if(!strcmp(modulename, registered_modules[i].name)) {
            foundmodule = 1;
            break;
        }
        i++;
    }

    if(!foundmodule) {
        fprintf(stderr, "Could not find module \"%s\"\n", modulename);
        return NULL;
    }

    fprintf(stderr, "Configuring module \"%s\"...\n", modulename);

    node = node->xmlChildrenNode;

    if(node == NULL || xmlIsBlankNode(node)) {
        fprintf(stderr, "Null or empty node in config read\n");
        return NULL;
    }

    do {
        if(node == NULL) break;
        if(xmlIsBlankNode(node)) continue;

        if(strcmp(node->name, "param") == 0) {
            param = (module_param_t *)calloc(1, sizeof(module_param_t));
            SET_PARM_STRING("name", param->name);
            SET_STRING(param->value);
            param->next = NULL;

            if(params == NULL)
                params = param;
            else {
                module_param_t *p = params;
                while(p->next != NULL)
                    p = p->next;
                p->next = param;
            }

        }
	} while ((node = node->next));

    module->open = registered_modules[i].open;
    module->params = params;

    if(!chain_start)
        return module;
    else {
        process_chain_element *current = chain_start;
        while(current->next)
            current = current->next;
        current->next = module;
        return chain_start;
    }
}

static void _parse_instance(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
    process_chain_element *chain = NULL;
    instance_t *instance = calloc(1, sizeof(instance_t));
    instance_t *prev;

    instance->max_queue_length = DEFAULT_MAX_QUEUE_LENGTH;

	do 
	{
		if (node == NULL) break;
		if (xmlIsBlankNode(node)) continue;

        if (strcmp(node->name, "maxqueuelength") == 0)
            SET_INT(instance->max_queue_length);
		if (strcmp(node->name, "module") == 0)
            chain = _parse_module(doc, node, chain);
	} while ((node = node->next));

    if(!chain) {
        fprintf(stderr, "Couldn't create processing chain\n");
        return;
    }

    instance->next = NULL;
    instance->output_chain = chain;
	instance->queue = calloc(1, sizeof(buffer_queue));
	thread_mutex_create(&instance->queue->lock);

    prev = config->instances;
    if(!prev)
        config->instances = instance;
    else {
        while(prev->next)
            prev = prev->next;

        prev->next = instance;
    }
}

static void _parse_input(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
    process_chain_element *chain = NULL;

	do 
	{
		if (node == NULL) break;
		if (xmlIsBlankNode(node)) continue;

		if (strcmp(node->name, "module") == 0)
            chain = _parse_module(doc, node, chain);
	} while ((node = node->next));

    config->input_chain = chain;
}

static void _parse_stream(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
	do 
	{
		if (node == NULL) break;
		if (xmlIsBlankNode(node)) continue;

		if (strcmp(node->name, "input") == 0)
			_parse_input(config, doc, node->xmlChildrenNode);
		else if (strcmp(node->name, "instance") == 0)
			_parse_instance(config, doc, node->xmlChildrenNode);
	} while ((node = node->next));
}

static void _parse_root(config_t *config, xmlDocPtr doc, xmlNodePtr node)
{
	do 
	{
		if (node == NULL) break;
		if (xmlIsBlankNode(node)) continue;
		
		if (strcmp(node->name, "background") == 0)
			SET_INT(config->background);
		else if (strcmp(node->name, "logpath") == 0)
			SET_STRING(config->logpath);
		else if (strcmp(node->name, "logfile") == 0)
			SET_STRING(config->logfile);
		else if (strcmp(node->name, "loglevel") == 0)
			SET_INT(config->loglevel);
        else if (strcmp(node->name, "consolelog") == 0)
            SET_INT(config->log_stderr);
		else if (strcmp(node->name, "stream") == 0)
			_parse_stream(config, doc, node->xmlChildrenNode);
	} while ((node = node->next));
}

void config_initialize(void)
{
	ices_config = (config_t *)calloc(1, sizeof(config_t));
	//_set_defaults(ices_config);
	srand(time(NULL));
}

void config_shutdown(void)
{
	if (ices_config == NULL) return;

	free(ices_config);
	ices_config = NULL;
}

int config_read(const char *fn)
{
	xmlDocPtr doc;
	xmlNodePtr node;

	if (fn == NULL || strcmp(fn, "") == 0) return -1;

	doc = xmlParseFile(fn);
	if (doc == NULL) return -1;

	node = xmlDocGetRootElement(doc);
	if (node == NULL || strcmp(node->name, "ices") != 0) 
	{
		xmlFreeDoc(doc);
		return 0;
	}

	_parse_root(ices_config, doc, node->xmlChildrenNode);

	xmlFreeDoc(doc);

	return 1;
}


/* FIXME: Write this again, for the new version 
void config_dump(void)
{
	config_t *c = ices_config;
	module_param_t *param;
	instance_t *i;

	fprintf(stderr, "ices config dump:\n");
	fprintf(stderr, "background = %d\n", c->background);
	fprintf(stderr, "logpath = %s\n", c->logpath);
	fprintf(stderr, "logfile = %s\n", c->logfile);
	fprintf(stderr, "loglevel = %d\n", c->loglevel);
	fprintf(stderr, "\n");
	fprintf(stderr, "stream_name = %s\n", c->stream_name);
	fprintf(stderr, "stream_genre = %s\n", c->stream_genre);
	fprintf(stderr, "stream_description = %s\n", c->stream_description);
	fprintf(stderr, "\n");
	fprintf(stderr, "playlist_module = %s\n", c->playlist_module);
	param = c->module_params;
	while(param)
	{
		fprintf(stderr, "module_param: %s = %s\n", param->name, param->value);
		param = param->next;
	}
	fprintf(stderr, "\ninstances:\n\n");

	i = c->instances;
	while (i) 
	{
		fprintf(stderr, "hostname = %s\n", i->hostname);
		fprintf(stderr, "port = %d\n", i->port);
		fprintf(stderr, "password = %s\n", i->password);
		fprintf(stderr, "mount = %s\n", i->mount);
		fprintf(stderr, "minimum bitrate = %d\n", i->min_br);
		fprintf(stderr, "nominal bitrate = %d\n", i->nom_br);
		fprintf(stderr, "maximum bitrate = %d\n", i->max_br);
		fprintf(stderr, "quality = %f\n", i->quality);
		fprintf(stderr, "managed = %d\n", i->managed);
		fprintf(stderr, "reencode = %d\n", i->encode);
		fprintf(stderr, "reconnect: %d times at %d second intervals\n", 
				i->reconnect_attempts, i->reconnect_delay);
		fprintf(stderr, "maxqueuelength = %d\n", i->max_queue_length);
		fprintf(stderr, "\n");

		i = i->next;
	}
	
	fprintf(stderr, "\n");
}
*/





