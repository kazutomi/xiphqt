/* config.h
 * - configuration, and global structures built from config
 *
 * $Id: config.h,v 1.10.2.2 2002/02/08 11:14:03 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

//#include "stream.h"
#include "process.h"
#include "thread/thread.h"

typedef struct _module_param_t
{
	char *name;
	char *value;

	struct _module_param_t *next;
} module_param_t;

struct _instance_t;

typedef struct _config_tag
{
	int background;
	char *logpath;
	char *logfile;
	int loglevel;
    int log_stderr;

	/* private */
	int log_id;
	int shutdown;
	cond_t queue_cond;
	cond_t event_pending_cond;
	mutex_t refcount_lock;
	mutex_t flush_lock;

    process_chain_element *input_chain;
    struct _instance_t *instances;
    
	struct _config_tag *next;
} config_t;

extern config_t *ices_config;

void config_initialise(void);
void config_shutdown(void);

int config_read(const char *filename);
void config_dump(void);

void config_free_instance(struct _instance_t *instance);
void config_free_params(module_param_t *params);

#endif /* __CONFIG_H__ */





