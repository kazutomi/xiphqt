/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Reference Library SOURCE CODE.      *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Ogg Reference Library SOURCE CODE IS (C) COPYRIGHT 1994-2004 *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: toplevel libogg include
 last mod: $Id: ogg.h 7296 2004-07-23 23:07:26Z arc $

 ********************************************************************/
#ifndef _OGG_H
#define _OGG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ogg2/ogg.h>

typedef struct {
} oggs_state;

typedef struct {
  int   num_media;
  int  *media;  /* This needs to point to a struct */
} oggs_media;

typedef struct {
  char  *name;
  int    num_fields;
  char **labels;
  void  *params;
} oggs_format;

typedef struct {
  char *protocol;
  void *address;
} oggs_method; 



/* An oggs_state object is required for all OggStream functionality.  */
/* The application must receive a new oggs_state with oggs_create and */
/* return it with oggs_destroy to prevent memory leakage.   To reuse  */
/* an oggs_state for new media the oggs_reset function may be called. */

extern oggs_state *oggs_create(void);
extern int oggs_reset(*oggs_state);
extern int oggs_destroy(*oggs_state);



/* Plugins are specified by requirements rather than name.  Input and */
/* output plugins are chosen based on the desired i/o method, while a */
/* codec plugin is chosen based on its ability to read the oggs_media */
/* provided.  Multiple codec plugins may be setup in series to output */
/* a desired media format, or an application can simply call a single */
/* init_codec with the desired format and allow the library to find a */
/* combination of codec plugins which can fufill the request. Null is */
/* returned if no plugins are available to complete a given request.  */

extern oggs_media *oggs_init_input(oggs_state  *oss,
                                   oggs_method *method);
extern oggs_media *oggs_init_codec(oggs_state  *oss,
                                   oggs_media  *media,
                                   oggs_format *format);
extern int oggs_init_output(oggs_state  *oss,
                            oggs_media  *media,
                            oggs_method *method);


#ifdef __cplusplus
}
#endif

#endif  /* _OGG_H */
