/*
    Copyright (C) 2004 Kor Nielsen
    Pulse driver code copyright (C) 2006 Lennart Poettering
    Unholy union copyright (C) 2007 Monty and Redhat Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "oss2pulse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>

#include "fcntl.h"

#include <sys/mman.h>
#include <sys/errno.h>

static int debuglevel = DEBUG_LEVEL_WARNING;
static int daemonized = 0;
static int dsp_fd = -1;
static int mixer_fd = -1;
static int sndstat_fd = -1;
static char *dsp_device = NULL;
static char *mixer_device = NULL;

void debug(const int level, const char *format, ...){
  if (level <= debuglevel){
    va_list ap;
    va_start(ap, format);
    if (daemonized){
      int dl;
      switch (level) {
      case DEBUG_LEVEL_CRITICAL:
	dl = LOG_CRIT;
	break;
      case DEBUG_LEVEL_ERROR:
	dl = LOG_ERR;
	break;
      case DEBUG_LEVEL_WARNING:
	dl = LOG_WARNING;
	break;
      case DEBUG_LEVEL_NOTICE:
	dl = LOG_NOTICE;
	break;
      case DEBUG_LEVEL_INFO:
	dl = LOG_INFO;
	break;
      default:
	dl = LOG_DEBUG;
      }
      vsyslog(dl, format, ap);
    
    }else if (level > DEBUG_LEVEL_WARNING)
      vprintf(format, ap);
    else
      vfprintf(stderr, format, ap);
    va_end(ap);
  }
}

void setscheduler(void){
  struct sched_param sched_param;
  
  if (sched_getparam(0, &sched_param) < 0) {
    debug(DEBUG_LEVEL_WARNING, "Scheduler getparam failed...\n");
    return;
  }
  sched_param.sched_priority = 10;
  if (!sched_setscheduler(0, SCHED_FIFO, &sched_param)) {
    debug(DEBUG_LEVEL_INFO, "Scheduler set to Round Robin with priority %i...\n", sched_param.sched_priority);
		return;
  }
  debug(DEBUG_LEVEL_WARNING, "Scheduler set to Round Robin with priority %i FAILED\n", sched_param.sched_priority);
}

/**
 * Displays help message. Calls exit() with value given in "result"
 * once information is printed.
 * @param version_only
 *    If this parameter is non-zero, only version will be displayed
 * @param result
 *    Return value to use when exiting at end of this function
 */
void print_help(int version_only, int result){
  printf(PACKAGE" version "VERSION"\n"
	 " 2004 Kor Nielsen\n"
	 " 2006 Lennart Poettering\n"
	 " 2007 Monty\n");
  if (!version_only) {
    // Print usage information
    printf("\nUsage:\n"
	   "   "PACKAGE" -h|v\n"
	   "   "PACKAGE" [-d] [-q] [-V] [-n <int>]\n"
	   "\n"
	   "Options:\n"
	   "     -h   --help\n"
	   "        Display this help message.\n"
	   "     -v   --version\n"
	   "        Display program version.\n"
	   "     -d   --detach\n"
	   "        Detach from console when we have our connection\n"
	   "        to Pulse server and FUSD. Once detached logging\n"
	   "        will happen to syslog using daemon facility.\n"
	   "     -n <int>   --devnum <int>\n"
	   "        Create device /dev/dsp<int> instead of /dev/dsp\n"
	   "        Allowed range is -1 .. 99.\n"
	   "     -q   --quiet\n"
	   "        Be more quiet\n"
	   "     -V   --verbose\n"
	   "        Be more verbose\n"
	   "\n"
	   "Exit values:\n"
	   "      0  Success in execution.\n"
	   "      1  Invalid commandline arguments.\n"
	   "      2  Failed to connect to Pulse.\n"
	   "      3  Failed to setup our device(s) under /dev/.\n"
	   "      4  Failed to detach from console\n"
	   "      5  Termination because we lost connection to Pulse\n");
  }
  exit(result);
}

void parse_arguments(int argc, char *argv[], int *detach, int *dspnum){
  int i = 0;
  *detach = 0; // Default, do not detach from console
  *dspnum = -1; // Default value, no device number!

  while (argv[++i]){
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_help(0, ERR_SUCCESS);

    } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
      print_help(1, ERR_SUCCESS);
    } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--detach") == 0) {
      *detach = 1;
    } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
      if (debuglevel > 0) debuglevel--;
    } else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--verbose") == 0) {
      if (debuglevel < 99) debuglevel++;
    } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--devnum") == 0) {
      
      if (argv[++i] && strspn(argv[i], "0123456789") == strlen(argv[i])) {
	sscanf(argv[i], "%d", dspnum);
	if (*dspnum < -1 || *dspnum > MAX_DEVICE_NUMBER){
	  printf("Device number is out of range.\n");
	  print_help(0, ERR_BADARG);
	}
      } else {
	// We got no device number or garbage
	printf("Invalid device number, expecting integer in range is -1 .. %d\n", MAX_DEVICE_NUMBER);
	print_help(0, ERR_BADARG);
      }
    } else {
      printf("Unknown option '%s'\n", argv[i]);
      print_help(0, ERR_BADARG);
    }
  }
}

int main(int argc, char *argv[]) {
  int dspnum, detach;
  
  parse_arguments(argc, argv, &detach, &dspnum);
  
  //mlockall(MCL_CURRENT | MCL_FUTURE);
  
  if (dspnum >= 0) {
    asprintf(&dsp_device, sizeof(dsp_device), "/dev/dsp%d", dspnum);
    asprintf(&mixer_device, sizeof(mixer_device), "/dev/mixer%d", dspnum);
  } else {
    dsp_device = strdup("/dev/dsp");
    mixer_device = strdup("/dev/mixer");
  }
  debug(DEBUG_LEVEL_INFO, "Creating DSP device '%s' and mixer device '%s'.\n", dsp_device, mixer_device);
  
  if(dsp_fd = fusd_register(dsp_device, "sound", dsp_device+5, 0666, NULL, &dsp_file_ops) < 0) {
    debug(DEBUG_LEVEL_CRITICAL, "Could not create DSP device\n");
    exit(ERR_FUSD_FAIL);
  }
  
  if(mixer_fd = fusd_register(mixer_device, "sound", mixer_device+5, 0666, NULL, &mixer_file_ops) < 0) {
    fusd_unregister(mixer_fd);
    debug(DEBUG_LEVEL_CRITICAL, "Could not create mixer device\n");
    exit(ERR_FUSD_FAIL);
  }

  if(sndstat_fd = fusd_register("/dev/sndstat", "sound", "sndstat", 0666, NULL, &sndstat_file_ops) < 0) {
    fusd_unregister(sndstat_fd);
    debug(DEBUG_LEVEL_CRITICAL, "Could not create sndstat device\n");
    exit(ERR_FUSD_FAIL);
  }

  if (detach) {
    if (daemon(0, 0) == -1) {
      debug(DEBUG_LEVEL_CRITICAL, "Could not detach from terminal\n");
      exit(ERR_DETACH_FAIL);
    } else {
      openlog(PACKAGE, 0, LOG_DAEMON);
      daemonized = 1;
    }
  }
  setscheduler();
  
  debug(DEBUG_LEVEL_INFO, "Running...\n");
  fusd_run();
  
  fusd_unregister(dsp_fd);
  fusd_unregister(mixer_fd);
  fusd_unregister(sndstat_fd);
  
  return 0;
}
