/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007-2011              *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: research-grade chirp extraction code
 last mod: $Id$

 ********************************************************************/

#include <cairo/cairo.h>
#define DT_iterations 0
#define DT_abserror 1
#define DT_percent 2
extern int toppad;
extern int leftpad;

extern void set_error_color(cairo_t *c, float err,float a);
extern void set_iter_color(cairo_t *cC, int ret, float a);
extern void to_png(cairo_t *c,char *base, char *name);
extern void destroy_page(cairo_t *c);
extern cairo_t *draw_page(char *title,
                          char *subtitle1,
                          char *subtitle2,
                          char *subtitle3,
                          char *xaxis_label,
                          char *yaxis_label,
                          char *legend_label,
                          int datatype);
extern void setup_graphs(int start_x_step,
                         int end_x_step, /* inclusive; not one past */
                         int x_major_d,

                         int start_y_step,
                         int end_y_step, /* inclusive; not one past */
                         int y_major_d,

                         int subtitles,
                         float fs);
