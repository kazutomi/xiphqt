/* plugin.h
 * Audio plugin API
 * Copyright (c) 2004 Brendan Cully <brendan@xiph.org>
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

#ifndef _ICES_PLUGIN_H_
#define _ICES_PLUGIN_H_ 1

typedef struct _ices_plugin {
  const char *name;

  void (*new_track)(void);
  int (*process)(int ilen, int16_t *il, int16_t *ir);

  struct _ices_plugin *next;
} ices_plugin_t;

ices_plugin_t *crossfade_plugin(int secs);

#endif
