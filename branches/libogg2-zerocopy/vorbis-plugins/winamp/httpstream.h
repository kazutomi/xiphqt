/*********************************************
*
*  httpstream.h
*
*  HTTP streaming support for OggVorbis by Aaron Porter <aaron@javasource.org>
*  Licensed under terms of the LGPL
*
*********************************************/

#ifndef _OGG_VORBIS_HTTP_STREAMING_
#define _OGG_VORBIS_HTTP_STREAMING_

void    httpInit();
void    httpSetHwnd(HWND hWnd);
void    httpSetProxy(const char *proxy);
void    httpShutdown();

int     isOggUrl(const char *url);

char *  httpGetTitle(const char *url);

void *  httpStartBuffering(const char *url, OggVorbis_File * input_file, BOOL showMessages);
void    httpStopBuffering();

void    setHttpVars();

#endif