


/*

  Example how the plugin interface works.(LINUX Open Sound System only!)
  Note: The callbacks setup_audio etc.. are done in a seperate thread!

  BUILD with:

  g++ -o vorbis -I/usr/X11R6/include -Iinclude \
  -I../../yaf/include -I../.. mainVorbis.cpp libvorbisplayer.a \
  lib/vorbisfile.a lib/libvorbis.a  \
  ../inputPlugin/libinputplugin.a ../outPlugin/liboutplugin.a \
  ../playerPlugin/libplayerplugin.a ../../yaf/shared/audio/libaudio.a \
  ../playerutil/dither/libdither.a \
  ../playerutil/libutil.a \
  -L/usr/X11R6/lib -lX11 -lXext -lpthread




 */


#include "vorbisPlugin.h"




int main(int argc, char** argv) {



  if (argc <= 1) {
    printf("Usage:\n\n");
    printf("%s filename\n\n",argv[0]);
    exit(0);
  }

  //
  // The order is important !!!!
  // 1. construct
  // 2. set Output
  // 3. open input
  // 4. set input
  // 
  // you cannot set the input _before_ the output 
  // in fact you can, but this gives you a segfault!
    
  VorbisPlugin* plugin=new VorbisPlugin();
  OutputStream* out=OutPlugin::createOutputStream(_OUTPUT_LOCAL);
  InputStream* in=InputPlugin::createInputStream(argv[1]);

  // The plugin does not do "open"
  in->open(argv[1]);

  // watch the order!
  plugin->setOutputPlugin(out);
  plugin->setInputPlugin(in);



  plugin->play();

  while(plugin->getStreamState() == _STREAM_STATE_NOT_EOF) {
    sleep(1);
  }
  cout << "plugin eof"<<endl;
  plugin->close();

  delete plugin;
  delete in;
  delete out;
  
}
  
