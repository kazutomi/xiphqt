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
#include <vorbis/vorbisfile.h>



/**
   callbacks from vorbisfile
*/
extern "C" {

extern size_t  fread_func  (void *ptr,size_t size,size_t nmemb, FILE *stream);
extern int     fseek_func  (FILE *stream, long offset, int whence);
extern int     fclose_func (FILE *stream);
extern long    ftell_func  (FILE *stream);

}



class VorbisPlugin : public PlayerPlugin {
  
  OggVorbis_File vf;


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
  int getSongLength(OggVorbis_File* vf);
  int init();

};


extern "C" {
  extern PlayerPlugin* getVorbisPlayer() {
    return new VorbisPlugin();
  }
}


#endif
