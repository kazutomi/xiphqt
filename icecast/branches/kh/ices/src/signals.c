/* signals.c
 * - signal handling/setup
 *
 * $Id: signals.c,v 1.4 2001/09/25 12:04:22 msmith Exp $
 *
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
#include <signal.h>

#include "thread/thread.h"

#include "cfgparse.h"
#include "stream.h"
#include "inputmodule.h"

#define MODULE "signals/"
#include "logging.h"

extern volatile int metadata_update_signalled;
int move_to_next_input;

void signal_usr1_handler(int signum __attribute__((unused)))
{
    /* LOG_INFO0("Metadata update requested"); */
    metadata_update_signalled = 1;

    signal(SIGUSR1, signal_usr1_handler);
}

void signal_usr2_handler(int signum __attribute__((unused)))
{
    /* LOG_INFO0("Switch to next input stream requested"); */

    move_to_next_input = 1;

	signal(SIGUSR2, signal_usr2_handler);
}

void signal_hup_handler(int signum __attribute__((unused)))
{
    LOG_INFO0("Flushing logs");
    log_flush(ices_config->log_id);

    ices_config->next_track = 1;
    signal(SIGHUP, signal_hup_handler);
}

void signal_int_handler(int signum __attribute__((unused)))
{
    /* LOG_INFO0("Shutdown requested..."); */
    ices_config->shutdown = 1;
    signal(SIGINT, signal_int_handler);
}


void signals_setup(void)
{
	signal(SIGINT, signal_int_handler);
	signal(SIGTERM, signal_int_handler);
	signal(SIGUSR1, signal_usr1_handler);
	signal(SIGUSR2, signal_usr2_handler);
	signal(SIGPIPE, SIG_IGN);
}


