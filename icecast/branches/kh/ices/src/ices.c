/* ices.c
 * - Main startup, thread launching, and cleanup code.
 *
 * $Id: ices.c,v 1.4 2002/01/29 09:20:27 msmith Exp $
 *
 * Copyright (c) 2001-2002 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#include "net/resolver.h"
#include "thread/thread.h"

#include "cfgparse.h"
#include "stream.h"
#include "signals.h"
#include "inputmodule.h"

#define MODULE "ices-core/"
#include "logging.h"

void start_processing ()
{
    thread_type *thread;

    thread = thread_create("input", input_loop, NULL, 0);
    if (thread == NULL)
        return;
    thread_join (thread);
}

int realtime_enabled = 0;


static void drop_priviledges()
{
    struct passwd *pw = NULL;

    if (ices_config->realtime)
    {
        struct sched_param param;
        int policy;

        pthread_getschedparam (pthread_self(), &policy, &param);
#ifdef HAVE_SCHED_GET_PRIORITY_MAX
        param . sched_priority = sched_get_priority_max (SCHED_RR);
#endif

        if (pthread_setschedparam (pthread_self(), SCHED_RR, &param))
            realtime_enabled = 0;
        else
            realtime_enabled = 1;
    }

    if (ices_config->user)
        pw = getpwnam (ices_config->user);

    if (pw)
    {
        setgid (pw->pw_gid);
        setgroups (0, NULL);
        setuid (pw->pw_uid);
    }
    else
    {
        setgid (getgid());
        /* setgroups (0, NULL); */
        setuid (getuid());
    }
}

static char logpath[FILENAME_MAX];

int main(int argc, char **argv)
{
	int log;

	if (argc != 2) 
	{
		fprintf(stderr, PACKAGE_STRING "\n"
				"  (c) Copyright 2002-2004 Karl Heyes <karl@xiph.org>\n\n"
				"Usage: \"ices config.xml\"\n");
		return 1;
	}

	config_initialize();

	/* right now you must have a config file, but we should probably
	** make it so you can specify all parameters on the commandline
	** too.
	*/
	if (config_read(argv[1]) < 0) 
	{
		fprintf(stderr, "Failed to read config file \"%s\"\n", argv[1]);
		goto fail;
	}
    if (ices_config->background)
    {
        int ret = 0;
        /* Start up new session, to lose old session and process group */
        switch (fork())
        {
        case 0: break; /* child continues */
        case -1: perror ("fork"); ret = -1;
        default: 
            exit (ret);
        }

        /* Disassociate process group and controlling terminal */
        setsid();

        /* Become a NON-session leader so that a */
        /* control terminal can't be reacquired */
        switch (fork())
        {
        case 0: break; /* child continues */
        case -1: perror ("fork"); ret = -1;
        default: 
            exit (ret);
        }
    }

	log_initialize();
    thread_initialize();
    shout_init();
	signals_setup();
    drop_priviledges();

	snprintf (logpath, FILENAME_MAX, "%s/%s", ices_config->logpath, 
			ices_config->logfile);
    if (ices_config->log_stderr)
        log = log_open_file(stderr);
    else
    {
	    log = log_open (logpath);
        if (log < 0)
            fprintf (stderr, "Failed to open log file %s\n", logpath);
        log_set_trigger (log, ices_config->logsize);
    }
	/* Set the log level, if requested - defaults to 2 (WARN) otherwise */
	if (ices_config->loglevel)
		log_set_level(log, ices_config->loglevel);

	ices_config->log_id = log;

	LOG_INFO0("Streamer version " PACKAGE_STRING);
	LOG_INFO1("libshout version %s ", shout_version(NULL, NULL,NULL));
    if (realtime_enabled)
        LOG_INFO0("realtime scheduling has been enabled");
    else
        LOG_INFO0("Unable to set realtime scheduling");

    if (ices_config->pidfile != NULL)
    {
        FILE *f = fopen (ices_config->pidfile, "w");
        if (f)
        {
            fprintf (f, "%i", getpid());
            fclose (f);
        }
    }

    start_processing ();

	LOG_INFO0("Shutdown in progress");

    if (ices_config->pidfile)
        remove (ices_config->pidfile);

	log_close(log);

 fail:
    shout_shutdown();
	config_shutdown();
	thread_shutdown();
	log_shutdown();

	return 0;
}


