/* pipe.c
 * two-way pipe abstraction
 * Copyright (c) 2005 Brendan Cully
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
 * $Id$
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "log.h"
#include "util.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

/* open a pipe to cmd, and store read and write file descriptors into
 *  rfd and wfd respectively. Returns pid of pipe process, or -1 on failure */
int ices_pipe(const char* cmd, int* rfd, int* wfd) {
  int pin[2], pout[2];
  int pid;
  
  if (pipe (pin) == -1) {
    ices_log_error("Error creating read pipe: %s", strerror(errno));
    return -1;
  }
  if (pipe (pout) == -1) {
    ices_log_error("Error creating write pipe: %s", strerror(errno));
    return -1;
  }
  
  if ((pid = fork()) == 0) {
    if (dup2 (pout[0], STDIN_FILENO) < 0 || dup2(pin[1], STDOUT_FILENO) < 0)
      _exit(127);
    close(pin[0]);
    close(pin[1]);
    close(pout[0]);
    close(pout[1]);
    close(STDERR_FILENO);
    
    setsid();
    
    execl("/bin/sh", "sh", "-c", cmd, NULL);
    _exit(127);
  }
  
  if (pid < 0) {
    close(pin[0]);
    close(pin[1]);
    close(pout[0]);
    close(pout[1]);
    ices_log_error("Error spawning pipe");
    return -1;
  }
  
  close(pin[1]);
  close(pout[0]);
  
  *rfd = pin[0];
  *wfd = pout[1];
  
  fcntl(*rfd, F_SETFD, FD_CLOEXEC);
  fcntl(*wfd, F_SETFD, FD_CLOEXEC);
  
  ices_log_debug("Opened pipe (pid %d) to %s", pid, cmd);
  
  return pid;  
}
