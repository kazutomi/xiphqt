/**
   @file ghost.h
   @brief Main codec file
 */

/* Copyright (C) 2005

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _GHOST_H
#define _GHOST_H

typedef struct {
   float basefreq;
   int harmonicstate;
   int transmitstatus;
} QuantSine;


#define PCM_BUF_SIZE 2048
#define SINUSOIDS 30
#define LENGTH 256
#define LPC_LENGTH 384
#define LPC_ORDER 40
#define ADVANCE 192
#define M_PI 3.1415926535897932384626433832795

#endif
