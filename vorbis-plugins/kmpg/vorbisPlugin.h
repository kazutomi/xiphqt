/*
  vorbis player plugin
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */


#ifndef __VORBISPLUGIN_H
#define __VORBISPLUGIN_H

#include <kmpg/playerPlugin/playerPlugin.h>
#include <kmpg/outPlugin/outputStream.h>
#include <kmpg/inputPlugin/inputStream.h>

#include <kmpg/playerutil/timeStamp.h>
#include <stdio.h>
#include <math.h>
#include <vorbis/codec.h>

#define _VORBIS_CONVSIZE   4096






class VorbisPlugin : public PlayerPlugin {
  
   /* take 8k out of the data segment, 
      not the stack */
  int16_t* convbuffer; 
  int convsize;


  ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
  ogg_stream_state os; /* take physical pages, weld into a logical
			  stream of packets */
  ogg_page         og; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet       op; /* one raw packet of data for decode */
  
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
			  settings */
  vorbis_comment   vc; /* struct that stores all the bitstream user comments */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */
  
  char *buffer;
  int  bytes;


  // END vorbis setup
  

  int lnoLength;
  int lfirst;
  int lAutoPlay;
  TimeStamp* timeDummy;

 public:
  VorbisPlugin();
  ~VorbisPlugin();

  void decoder_loop();
  int seek(int second);
  void config(char* key, char* value);
 
 private:
  int getSongLength();
  int init();

};


extern "C" {
  extern PlayerPlugin* getVorbisPlayer() {
    return new VorbisPlugin();
  }
}


#endif
