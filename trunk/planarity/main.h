/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <gtk/gtk.h>

#ifdef ENABLE_NLS
#include <locale.h>
#include <libintl.h>

#define GT_DOMAIN X_GT_DOMAIN(UGT_DOMAIN)
#define X_GT_DOMAIN(x) XX_GT_DOMAIN(x)
#define XX_GT_DOMAIN(x) #x

#define GT_DIR X_GT_DIR(UGT_DIR)
#define X_GT_DIR(x) XX_GT_DIR(x)
#define XX_GT_DIR(x) #x

#define _(x) gettext(x)
#else
#define _(x) x
#endif

extern char *boarddir;
extern char *statedir;
extern char *fontface;
extern char *version;

extern Gameboard *gameboard;

extern void request_resize(int width, int height);
extern void set_font(cairo_t *c, float w, float h, int slant, int bold);
