/*
 *   Wavelet Denoise filter for GIMP - The GNU Image Manipulation Program
 *
 *   Copyright (C) 2008 Monty
 *   Code based on research by Crystal Wagner and Prof. Ivan Selesnik, 
 *   Polytechnic University, Brooklyn, NY
 *   See: http://taco.poly.edu/selesi/DoubleSoftware/
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include <string.h>
#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/*
 * Constants...
 */

#define PLUG_IN_PROC    "plug-in-denoise"
#define PLUG_IN_BINARY  "denoise"
#define PLUG_IN_VERSION "14 Dec 2008"

/*
 * Local functions...
 */

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **returm_vals);

static void     denoise        (GimpDrawable *drawable);
static void     denoise_pre    (GimpDrawable *drawable);

static gboolean denoise_dialog (GimpDrawable *drawable);
static int      denoise_work(int w, int h, int bpp, guchar *buffer, int progress, int(*interrupt)(void));

static void     preview_update (GtkWidget  *preview, GtkWidget *dialog);

/*
 * Globals...
 */

const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run    /* run_proc   */
};

typedef struct
{
  float filter;
  int soft;
  int multiscale;
  float f1;
  float f2;
  float f3;
  float f4;
  int lowlight;
  float lowlight_adj;
} DenoiseParams;

static DenoiseParams denoise_params =
{
  20, 0, 0,
  0.,0.,0.,0.,
  0,0.
};

static GtkWidget    *preview;
static GtkObject    *madj[4];
static GtkObject    *ladj[1];
static guchar *preview_cache_buffer=NULL;
static int     preview_cache_x;
static int     preview_cache_y;
static int     preview_cache_w;
static int     preview_cache_h;

static int     variance_median=0;

MAIN ()

#include "wavelet.c"

static void
query (void)
{
  static const GimpParamDef   args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "Interactive, non-interactive"      },
    { GIMP_PDB_IMAGE,    "image",         "Input image"                       },
    { GIMP_PDB_DRAWABLE, "drawable",      "Input drawable"                    },
    { GIMP_PDB_FLOAT,    "filter",        "Denoise filter strength"           },
    { GIMP_PDB_INT32,    "soft",          "Use soft thresholding"             },
    { GIMP_PDB_INT32,    "multiscale",    "Enable multiscale adjustment"      },
    { GIMP_PDB_FLOAT,    "f1",            "Fine detail adjust"                },
    { GIMP_PDB_FLOAT,    "f2",            "Detail adjust"                     },
    { GIMP_PDB_FLOAT,    "f3",            "Mid adjust"                        },
    { GIMP_PDB_FLOAT,    "f4",            "Coarse adjust"                     },
    { GIMP_PDB_INT32,    "lowlight",      "Low light noise mode"              },
    { GIMP_PDB_FLOAT,    "lowlight_adj",  "Low light threshold adj"           },

  };

  gimp_install_procedure (PLUG_IN_PROC,
                          "Denoise filter",
			  "This plugin uses directional wavelets to remove random "
			  "noise from an image; at an extreme produces an airbrush-like"
			  "effect.",
                          "Monty <monty@xiph.org>",
                          "Copyright 2008 by Monty",
                          PLUG_IN_VERSION,
                          "_Denoise...",
                          "RGB*, GRAY*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_plugin_menu_register (PLUG_IN_PROC, "<Image>/Filters/Montypak");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  static GimpParam   values[1];    /* Return values */
  GimpRunMode        run_mode;  /* Current run mode */
  GimpPDBStatusType  status;    /* Return status */
  GimpDrawable      *drawable;  /* Current image */

  /*
   * Initialize parameter data...
   */

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*
   * Get drawable information...
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);

  /* math that has to be done ahead of time */
  denoise_pre(drawable);

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &denoise_params);

      /*
       * Get information from the dialog...
       */
      if (!denoise_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams != 12)
        status = GIMP_PDB_CALLING_ERROR;
      else{
        denoise_params.filter = param[3].data.d_float;
	denoise_params.soft   = param[4].data.d_int32;
	denoise_params.multiscale = param[5].data.d_int32;
        denoise_params.f1 = param[6].data.d_float;
        denoise_params.f2 = param[7].data.d_float;
        denoise_params.f3 = param[8].data.d_float;
        denoise_params.f4 = param[9].data.d_float;
        denoise_params.lowlight = param[10].data.d_int32;
        denoise_params.lowlight_adj = param[11].data.d_float;
      }
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &denoise_params);
      break;

    default:
      status = GIMP_PDB_CALLING_ERROR;
      break;
    }

  if (status == GIMP_PDB_SUCCESS)
    {
      if ((gimp_drawable_is_rgb (drawable->drawable_id) ||
           gimp_drawable_is_gray (drawable->drawable_id)))
        {
          /*
           * Run!
           */
          denoise (drawable);

          /*
           * If run mode is interactive, flush displays...
           */
          if (run_mode != GIMP_RUN_NONINTERACTIVE)
            gimp_displays_flush ();

          /*
           * Store data...
           */
          if (run_mode == GIMP_RUN_INTERACTIVE)
            gimp_set_data (PLUG_IN_PROC,
                           &denoise_params, sizeof (DenoiseParams));
        }
      else
        status = GIMP_PDB_EXECUTION_ERROR;
    }

  /*
   * Reset the current run status...
   */
  values[0].data.d_status = status;

  /*
   * Detach from the drawable...
   */
  gimp_drawable_detach (drawable);
}

static void denoise (GimpDrawable *drawable){

  GimpPixelRgn  src_rgn;        /* Source image region */
  GimpPixelRgn  dst_rgn;        /* Destination image region */
  gint          x1,x2;
  gint          y1,y2;
  gint          w;
  gint          h;
  gint          bpp;
  guchar       *buffer;

  gimp_drawable_mask_bounds (drawable->drawable_id,
                             &x1, &y1, &x2, &y2);

  w = x2 - x1;
  h = y2 - y1;
  bpp    = gimp_drawable_bpp (drawable->drawable_id);

  /*
   * Setup for filter...
   */

  gimp_pixel_rgn_init (&src_rgn, drawable,
                       x1, y1, w, h, FALSE, FALSE);
  gimp_pixel_rgn_init (&dst_rgn, drawable,
                       x1, y1, w, h, TRUE, TRUE);

  /***************************************/
  buffer = g_new (guchar, w * h * bpp);
  gimp_pixel_rgn_get_rect (&src_rgn, buffer, x1, y1, w, h);
  denoise_work(w,h,bpp,buffer,1,NULL);
  gimp_pixel_rgn_set_rect (&dst_rgn, buffer, x1, y1, w, h);
  /**************************************/


  /*
   * Update the screen...
   */

  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->drawable_id, TRUE);
  gimp_drawable_update (drawable->drawable_id, x1, y1, w, h);
		      
  g_free(buffer);
}

static void dialog_soft_callback (GtkWidget *widget,
                          gpointer   data)
{
  denoise_params.soft = (GTK_TOGGLE_BUTTON (widget)->active);
  if(denoise_params.filter>0.)
    gimp_preview_invalidate (GIMP_PREVIEW (preview));
}


static void dialog_multiscale_callback (GtkWidget *widget,
                          gpointer   data)
{
  denoise_params.multiscale = (GTK_TOGGLE_BUTTON (widget)->active);
  gimp_scale_entry_set_sensitive(madj[0],denoise_params.multiscale);
  gimp_scale_entry_set_sensitive(madj[1],denoise_params.multiscale);
  gimp_scale_entry_set_sensitive(madj[2],denoise_params.multiscale);
  gimp_scale_entry_set_sensitive(madj[3],denoise_params.multiscale);
  if(denoise_params.f1!=0. ||
     denoise_params.f2!=0. ||
     denoise_params.f3!=0. ||
     denoise_params.f4!=0.)
    gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void dialog_lowlight_callback (GtkWidget *widget,
				      gpointer   data)
{
  denoise_params.lowlight = (GTK_TOGGLE_BUTTON (widget)->active);
  gimp_scale_entry_set_sensitive(ladj[0],denoise_params.lowlight);
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void dump_cache(GtkWidget *widget, gpointer dummy){
  if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))){
    if(preview_cache_buffer)
      g_free(preview_cache_buffer);
    preview_cache_buffer=NULL;
  }
}

static void decache_on_toggle(GtkWidget *widget, gpointer dummy){
  g_signal_connect (widget, "toggled",
                    G_CALLBACK (dump_cache),
		    NULL);
}

static gboolean denoise_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *button;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new ("Denoise", PLUG_IN_BINARY,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC,
			    
                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,
			    
                            NULL);
  
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  preview = gimp_drawable_preview_new (drawable, NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (preview_update),
                    dialog);
  gtk_container_foreach(GTK_CONTAINER(gimp_preview_get_controls(GIMP_PREVIEW(preview))),
			decache_on_toggle,NULL);

  /* Main filter strength adjust */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              "_Denoise", 300, 0,
                              denoise_params.filter,
                              0, 100, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &denoise_params.filter);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);


  /* Threshold shape */
  button = gtk_check_button_new_with_mnemonic ("So_ft thresholding");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                denoise_params.soft);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_soft_callback),
                    NULL);

  /* Low-light mode */
  button = gtk_check_button_new_with_mnemonic ("_Low-light noise mode");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                denoise_params.lowlight);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_lowlight_callback),
                    NULL);

  /* Subadjustments for lowlight mode */
  table = gtk_table_new (1, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* low light threshold adj */
  ladj[0] = adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 0,
				       "_Threshold adjust:", 300, 0,
					denoise_params.lowlight_adj,
					-100, +100, 1, 10, 0,
					TRUE, 0, 0,
					NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &denoise_params.lowlight_adj);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);
  gimp_scale_entry_set_sensitive(ladj[0],denoise_params.lowlight);

  /* multiscale adjust select */
  button = gtk_check_button_new_with_mnemonic ("Multiscale _adjustment");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                denoise_params.multiscale);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_multiscale_callback),
                    NULL);

  /* Subadjustments for multiscale */
  table = gtk_table_new (4, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* fine detail adjust */
  madj[0] = adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 0,
				       "_Very fine denoise:", 300, 0,
				       denoise_params.f1,
				       -100, +100, 1, 10, 0,
				       TRUE, 0, 0,
				       NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &denoise_params.f1);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* detail adjust */
  madj[1] = adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 1,
				       "_Fine denoise:", 300, 0,
				       denoise_params.f2,
				       -100, 100, 1, 10, 0,
				       TRUE, 0, 0,
				       NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &denoise_params.f2);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* mid adjust */
  madj[2] = adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 2,
				       "_Mid denoise:", 300, 0,
				       denoise_params.f3,
				       -100, 100, 1, 10, 0,
				       TRUE, 0, 0,
				       NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &denoise_params.f3);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* Coarse adjust */
  madj[3] = adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 3,
					"_Coarse denoise:", 300, 0,
					denoise_params.f4,
					-100, 100, 1, 10, 0,
					TRUE, 0, 0,
					NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_float_adjustment_update),
                    &denoise_params.f4);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  gimp_scale_entry_set_sensitive(madj[0],denoise_params.multiscale);
  gimp_scale_entry_set_sensitive(madj[1],denoise_params.multiscale);
  gimp_scale_entry_set_sensitive(madj[2],denoise_params.multiscale);
  gimp_scale_entry_set_sensitive(madj[3],denoise_params.multiscale);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  if(preview_cache_buffer)
    g_free(preview_cache_buffer);
  preview_cache_buffer=NULL;
  return run;
}

static int denoise_active_interruptable;
static int denoise_active_interrupt;

static int check_recompute(){
  while(gtk_events_pending())
    gtk_main_iteration ();
  return denoise_active_interrupt;
}

static int set_busy(GtkWidget *preview, GtkWidget *dialog){
  GdkDisplay    *display = gtk_widget_get_display (dialog);
  GdkCursor     *cursor = gdk_cursor_new_for_display (display, GDK_WATCH);
  gdk_window_set_cursor(preview->window, cursor);
  gdk_window_set_cursor(gimp_preview_get_area(GIMP_PREVIEW(preview))->window, cursor);
  gdk_window_set_cursor(dialog->window, cursor);
  gdk_cursor_unref(cursor);
}

static void preview_update (GtkWidget *preview, GtkWidget *dialog){

  GimpDrawable *drawable;
  GimpPixelRgn  rgn;            /* previw region */
  gint          x1, y1;
  gint          w,h;
  guchar       *buffer=NULL;
  gint          bpp;        /* Bytes-per-pixel in image */

  /* because we allow async event processing, a stray expose of the
     GimpPreviewArea will cause it reload the original unfiltered
     data, which is annoying when doing small parameter tweaks.  Make
     sure the previous run's data is blitted in if it can be */
  drawable = gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));
  bpp = gimp_drawable_bpp (drawable->drawable_id);
  gimp_preview_get_position (GIMP_PREVIEW(preview), &x1, &y1);
  gimp_preview_get_size (GIMP_PREVIEW(preview), &w, &h);

  if(preview_cache_buffer &&
     x1 == preview_cache_x &&
     y1 == preview_cache_y &&
     w == preview_cache_w &&
     h == preview_cache_h){
    
    gimp_preview_draw_buffer (GIMP_PREVIEW(preview), preview_cache_buffer, w*bpp);
  }else{
    if(preview_cache_buffer)
      g_free(preview_cache_buffer);
    preview_cache_buffer=NULL;
  }

  denoise_active_interrupt=1;
  if(denoise_active_interruptable) return;
  denoise_active_interruptable = 1;

  while(denoise_active_interrupt){
    denoise_active_interrupt=0;

    set_busy(preview,dialog);

    gimp_preview_get_position (GIMP_PREVIEW(preview), &x1, &y1);
    gimp_preview_get_size (GIMP_PREVIEW(preview), &w, &h);
    gimp_pixel_rgn_init (&rgn, drawable,
			 x1, y1, w, h,
			 FALSE, FALSE);
    if(buffer) g_free (buffer);
    buffer = g_new (guchar, w * h * bpp);
    gimp_pixel_rgn_get_rect (&rgn, buffer, x1, y1, w, h);
    if(!denoise_work(w,h,bpp,buffer,0,check_recompute)){
      gimp_preview_draw_buffer (GIMP_PREVIEW(preview), buffer, w*bpp);
      gimp_drawable_flush (drawable);
    }
  }
  
  if(preview_cache_buffer)
    g_free(preview_cache_buffer);
  preview_cache_buffer = buffer;
  preview_cache_x = x1;
  preview_cache_y = y1;
  preview_cache_w = w;
  preview_cache_h = h;

  denoise_active_interruptable = 0;
}

#include "varmean.c"
#define clamp(x) ((x)<0?0:((x)>255?255:(x)))

static void compute_luma(guchar *buffer, guchar *luma, int width, int height, int planes){
  int i;
  switch(planes){
  case 1:
    memcpy(luma,buffer,sizeof(*luma)*width*height);
    break;
  case 2: 
    for(i=0;i<width*height;i++)
      luma[i]=buffer[i<<1];
    break;
  case 3:
    for(i=0;i<width*height;i++)
      luma[i]=buffer[i*3]*.2126 + buffer[i*3+1]*.7152 + buffer[i*3+2]*.0722;
    break;
  case 4:
    for(i=0;i<width*height;i++)
      luma[i]=buffer[i*4]*.2126 + buffer[i*4+1]*.7152 + buffer[i*4+2]*.0722;
    break;
  }
}

static void denoise_pre(GimpDrawable *drawable){
  /* find the variance median for the whole image, not just preview, not just the selection */

  GimpPixelRgn  rgn;
  gint          w = gimp_drawable_width(drawable->drawable_id);
  gint          h = gimp_drawable_height(drawable->drawable_id);
  gint          bpp = gimp_drawable_bpp (drawable->drawable_id);
  guchar       *buffer;
  guchar       *luma;
  float       *v;
  long          d[256];
  int           i,a,a2,med;

  gimp_pixel_rgn_init (&rgn, drawable,
                       0, 0, w, h, FALSE, FALSE);
  buffer = g_new (guchar, w * h * bpp);
  luma = g_new (guchar, w * h);
  gimp_pixel_rgn_get_rect (&rgn, buffer, 0, 0, w, h);
  compute_luma(buffer,luma,w,h,bpp);
  g_free(buffer);

  /* collect var/mean on the luma plane */
  v = g_new(float,w*h);
  collect_var(luma, v, NULL, w, h, 5);
  g_free(luma);

  a=0,a2=0;
  memset(d,0,sizeof(d));
    
  for(i=0;i<w*h;i++){
    int val = clamp(sqrt(v[i]));
    d[val]++;
  }
  g_free(v);

  for(i=0;i<256;i++)
    a+=d[i];
    
  variance_median=256;
  for(i=0;i<256;i++){
    a2+=d[i];
    if(a2>=a*.5){
      variance_median=i+1;
      break;
    }
  }
}

static int denoise_work(int width, int height, int planes, guchar *buffer, int pr, int(*check)(void)){
  int i;
  float T[16];
  guchar *mask=NULL;

  if(denoise_params.lowlight){
    float l = denoise_params.lowlight_adj*.01;
    int med = variance_median*8;
    if(pr)gimp_progress_init( "Masking luma...");

    mask = g_new(guchar,width*height);
    compute_luma(buffer,mask,width,height,planes);

    if(l>0){
      med += (255-med)*l;
    }else{
      med += med*l;
    }

    if(check && check()){
      g_free(mask);
      return 1;
    } 

    /* adjust luma into mask form */
    for(i=0;i<width*height;i++){
      if(mask[i]<med){
	mask[i]=255;
      }else if(mask[i]>=med*2){
	mask[i]=0;
      }else{
	float del = (float)(mask[i]-med)/med;
	mask[i]=255-255*del;
      }
    }

    if(check && check()){
      g_free(mask);
      return 1;
    } 

    /* smooth the luma plane */
    for(i=0;i<16;i++)
      T[i]=10.;
    if(wavelet_filter(width, height, 1, mask, NULL, pr, T, denoise_params.soft, check)){
      g_free(mask);
      return 1;
    }

    if(pr)gimp_progress_end();
  }

  if(pr)gimp_progress_init( "Denoising...");
  
  for(i=0;i<16;i++)
    T[i]=denoise_params.filter*.2;
  
  if(denoise_params.multiscale){
    T[0]*=(denoise_params.f1+100)*.01;
    T[1]*=(denoise_params.f2+100)*.01;
    T[2]*=(denoise_params.f3+100)*.01;
    for(i=3;i<16;i++)
      T[i]*=(denoise_params.f4+100)*.01;
  }
  
  if(wavelet_filter(width, height, planes, buffer, mask, pr, T, denoise_params.soft, check)){
    if(mask)g_free(mask);
    return 1;
  }

  if(mask)
    g_free(mask);
 
  return 0;
}

