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

#include <mpeglib/decoder/decoderPlugin.h>
#include <mpeglib/output/outputStream.h>
#include <mpeglib/input/inputStream.h>

#include <stdio.h>
#include <math.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

/**
   callbacks from vorbisfile
*/
extern "C" {

extern size_t  fread_func  (void *ptr,size_t size,size_t nmemb, void *stream);
extern int     fseek_func  (void *stream, int64_t offset, int whence);
extern int     fclose_func (void *stream);
extern long    ftell_func  (void *stream);

}



class VorbisPlugin : public DecoderPlugin {
  
  OggVorbis_File vf;


  // END vorbis setup
  

  int lnoLength;
  int lAutoPlay;
  TimeStamp* timeDummy;
  char* pcmout;           // temporay pcm buffer
  int last_section;
  int current_section;
  int lshutdown; 

 public:
  VorbisPlugin();
  ~VorbisPlugin();

  void decoder_loop();
  int seek_impl(int second);
  void config(char* key, char* value,void* user_data);
 
 private:
  int processVorbis(vorbis_info* vi,vorbis_comment* comment);
  int getTotalLength();
  int init();

};


extern "C" {
  extern DecoderPlugin* getVorbisPlayer() {
    return new VorbisPlugin();
  }
}


#endif
