/*
    Copyright (C) 2004 Kor Nielsen
    Pulse driver code copyright (C) 2006 Lennart Poettering
    Unholy union copyright (C) 2007 Monty and Red Hat, Inc.

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

static const char sndstat[] =
  "Sound Driver:3.8s2++-070109 (PulseAudio OSS Emulation Daemon)\n"
  "Kernel: POSIX\n"
  "Config options: 0\n"
  "\n"
  "Installed drivers:\n"
  "Type 255: PulseAudio OSS Emulation Daemon\n"
  "\n"
  "Card config:\n"
  "PulseAudio OSS Emulation Daemon\n"
  "\n"
  "Audio devices:\n"
  "0: PulseAudio OSS Emulation Daemon\n"
  "\n"
  "Synth devices: NOT ENABLED IN CONFIG\n"
  "\n"
  "Midi devices:\n"
  "\n"
  "Timers:\n"
  "\n"
  "Mixers:\n"
  "0: PulseAudio OSS Emulation Daemon\n";

static int sndstat_success(struct fusd_file_info *file){
  return 0;
}

static ssize_t sndstat_read(struct fusd_file_info *file, 
			    char *user_buffer, 
			    size_t user_length, 
			    loff_t *offset) {
  int retval = 0;
  
  if (*offset < sizeof(sndstat)) {
    ssize_t tocopy = sizeof(sndstat) - *offset;
    if(tocopy > user_length) tocopy = user_length;
    memcpy(user_buffer, sndstat + *offset, tocopy);
    retval = tocopy;
    *offset += tocopy;
  }
  
  return retval;
}

static int sndstat_mmap(struct fusd_file_info *file, 
			int offset, 
			size_t length, 
			int flags, 
			void** addr, 
			size_t* out_length){
  *addr = sndstat;
  *out_length = sizeof(sndstat);
  
  return 0;
}

struct fusd_file_operations sndstat_file_ops = 
{
        open: sndstat_success,
        read: sndstat_read,
        close: sndstat_success,
        mmap: sndstat_mmap
};
