/*
 * core.c, "the core" of the DryIce source client for Icecast2
 *
 * Copyright (c) 2004 Arc Riley <arc@xiph.org>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   last mod: $Id: core.c,v 1.2 2004/03/04 06:35:16 arc Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <libgen.h>
#include <shout/shout.h>

void
usage (char *pname)
{
  fprintf (stderr,
           "Usage: %s <options>\n"
           " -c <file>            location of configuration file to use\n",
           (char*)basename(pname));
  exit (1);
}

int main(int argc, char **argv)
{
  FILE *config;
  char config_filename[256]="dryice.conf";

  int c;


/* This obviously needs more work 

  while ((c = getopt (argc, argv, "c:")) != EOF) {
    switch (c) {
      case 'c':
        config_filename = optarg;
        break;
      default:
        usage (argv[0]);
        break;
    }
  }


  if ((config = fopen(config_filename, "r")) == NULL) {
    printf("Could not open configuration file\n", config_filename);
    return -1;
  } 
  fclose(config);
*/

  /* BIG TODO

  Ok after the configs are read, which for now will be hard-coded 
  because I'm lazy and want to leave config file parsing for someone
  else, we'll open the input module(s), get the parameters for encoding,
  then open the codec module(s) with those parameters.

  Take the packets from the codec(s), toss them into an Ogg, ship the 
  whole thing out to libtheora with raw packets.  There is no need to 
  sleep since the stream is already running realtime, and thus, no need
  to do threading/etc as IceS2 does.  We'll basically just use libshout2
  for it's ability to initiate and pass data to Icecast2.

  */

  return 0;
}
