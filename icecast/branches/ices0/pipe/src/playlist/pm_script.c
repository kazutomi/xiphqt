/* pm_script.c
 * - playlist module for external scripts (á la IceS 2.0)
 * - based on playlist_script.c from IceS 2.0 by 
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * Copyright (C) 2005 Ville Koskinen <ville.koskinen@iki.fi> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "definitions.h"
#include <stdio.h>
#include <stdlib.h>

#define STR_BUFFER 1024

static char *playlist_metadata = NULL;
static char *cmd = NULL;

extern ices_config_t ices_config;

/* Private function declarations */
static char* playlist_script_get_next (void);
static void playlist_script_shutdown (void);
static char* playlist_script_get_metadata (void);

/* Global function definitions */

/* Initialize the script playlist handler */
int
ices_playlist_script_initialize (playlist_module_t* pm)
{
  ices_log_debug ("Initializing script playlist handler...");

  if (!pm->module) {
    ices_log_error ("No playlist script defined");
    return 0;
  }
  
  cmd = pm->module;
  /* make path relative to module dir */
  if (cmd[0] != '/' && 
      !(cmd[0] == '.' && (cmd[1] == '/' || (cmd[1] == '.' && cmd[2] == '/'))))
  {
      cmd = malloc (strlen(pm->module) + strlen(ICES_MODULEDIR) + 2);
      if (cmd)
        sprintf (cmd, "%s/%s", ICES_MODULEDIR, pm->module);
  } else
    cmd = strdup(pm->module);
    
  if (!cmd) {
    ices_log_error("Could not allocate memory for playlist path");
    return 0;
  }

  pm->get_next = playlist_script_get_next;
  pm->get_metadata = playlist_script_get_metadata;
  pm->get_lineno = NULL;
  pm->shutdown = playlist_script_shutdown;

  return 1;
}

static char *
playlist_script_get_next (void)
{
  char *filename = NULL, *metadata = NULL;
  FILE *pipe;
  int i = 0;

  filename = malloc(STR_BUFFER);
  metadata = malloc(STR_BUFFER);
	
  pipe = popen(cmd, "r");

  if (!pipe) {
	  ices_log_error ("Couldn't open pipe to program \"%s\"", cmd);
	  return NULL;
  }

  if (fgets(filename, STR_BUFFER, pipe) == NULL) {
	  ices_log_error ("Couldn't read filename from pipe to program \"%s\"", cmd);
	  free(filename); filename = NULL;
	  free(metadata); metadata = NULL;
	  pclose(pipe);
	  return NULL;
  }

  if (fgets(metadata, STR_BUFFER, pipe) == NULL) {
	  /* This is non-fatal. */
	  ices_log_debug ("No metadata received from pipe to program \"%s\"", cmd);
	  free(metadata); metadata = NULL;
  }
	  
  pclose(pipe);

  if (filename[0] == '\n' || (filename[0] == '\r' && filename[1] == '\n')) {
		ices_log_error ("Got newlines instead of filename from program \"%s\"", cmd);
		free(filename); filename = NULL;
		free(metadata); metadata = NULL;
		pclose(pipe);
		return NULL;
  }

  /* Remove linefeeds etc. */
  i = 0;
  while (filename[i]) {
	  if (filename[i] == '\r' || filename[i] == '\n') {
		  filename[i] = '\0';
		  break;
	  }
	  i++;
  }
  i = 0;
  while (metadata && metadata[i]) {
	  if (metadata[i] == '\r' || metadata[i] == '\n') {
		  metadata[i] = '\0';
		  break;
	  }
	  i++;
  }

  if (playlist_metadata)
	  free(playlist_metadata);
  if (metadata) 
	  playlist_metadata = metadata;
  else
	  playlist_metadata = NULL;
  
  ices_log_debug ("Script playlist handler serving: %s [%s]", ices_util_nullcheck (filename), ices_util_nullcheck(playlist_metadata));

  return filename;
}

/* Return the file metadata. */
static char*
playlist_script_get_metadata (void) 
{
	if (playlist_metadata)
		return playlist_metadata;
	return NULL;
}

/* Shutdown the script playlist handler */
static void
playlist_script_shutdown (void)
{
  if (cmd)
    free (cmd);
  if (playlist_metadata)
    free(playlist_metadata);
}

