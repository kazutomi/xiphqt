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
 last mod: $Id: oggs.h 7296 2004-07-23 23:07:26Z arc $

 ********************************************************************/
#ifndef _OGGS_H
#define _OGGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ogg2/ogg.h>

typedef struct {
/* This is obviously not finished.

I'm not even sure what this will be useful for, yet.  May be removed 
later if deemed unnessesary, but I'm sure a good use will come up.
*/

} oggs_state;

typedef struct {

/* This is obviously not finished.

The media will contain either one or multiple ogg_packet objects (I'm 
thinking one is enough - multiple just adds latency unnessesarily since 
buffering is done by the input/output plugins), a current time offset, 
the format (oggs_format), and other information nessesary for matching.
*/

} oggs_media;

typedef struct {
  char  *name;
  int    num_fields;
  char **labels;
  char **params;
} oggs_format;

typedef struct {
  char *protocol;
  void *address;
} oggs_method; 

typedef struct {
  oggs_media   *input;
  ogg_uint32_t  last_time;
  ogg_uint32_t  total_time;
  char         *table;
/* This is a table of 2-bit fields, each representing one second of
   total_time, as follows: 00 = Unknown State or Missing
                           01 = Known to be available or Incomplete
                           10 = Bad or Damaged section of stream
                           11 = Cached and verified good
   This is intended to give a visual scope for not only damaged streams
   but also for streaming buffers/caches and encoding status. */
} oggs_seekbar;


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

extern int oggs_init_input( oggs_state    *oss,
                            oggs_method   *method,
                            oggs_seekbar  *seek,
                            oggs_media   **output );

extern int oggs_init_codec( oggs_state   *oss,
                            int           num_input,
                            oggs_media  **input,
                            oggs_format  *format,
                            oggs_media  **output );

extern int oggs_init_output( oggs_state   *oss,
                             int           num_input,
                             oggs_media  **input,
                             oggs_method  *method);


#ifdef __cplusplus
}
#endif

#endif  /* _OGGS_H */
