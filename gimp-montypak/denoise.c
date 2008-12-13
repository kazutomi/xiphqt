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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/*
 * Constants...
 */

#define PLUG_IN_PROC    "plug-in-denoise"
#define PLUG_IN_BINARY  "denoise"
#define PLUG_IN_VERSION "28 Oct 2008"

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

static gboolean denoise_dialog (GimpDrawable *drawable);
static void     denoise_work(int w, int h, int bpp, guchar *buffer, int progress);

static void     preview_update (GimpPreview  *preview);

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

} DenoiseParams;

static DenoiseParams denoise_params =
{
  2, 0, 0,
  0.,0.,0.,0.,

};

static GtkWidget    *preview;
static GtkObject    *madj[4];

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
    { GIMP_PDB_FLOAT,    "filter",        "Global denoise filter strength"    },
    { GIMP_PDB_INT32,    "soft",          "Use soft thresholding"             },
    { GIMP_PDB_INT32,    "multiscale",    "Enable multiscale adjustment"      },
    { GIMP_PDB_FLOAT,    "f1",            "Fine detail denoise"               },
    { GIMP_PDB_FLOAT,    "f2",            "Detail denoise"                    },
    { GIMP_PDB_FLOAT,    "f3",            "Mid denoise"                       },
    { GIMP_PDB_FLOAT,    "f4",            "Coarse denoise"                    },
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
  GimpRunMode        run_mode;  /* Current run mode */
  GimpPDBStatusType  status;    /* Return status */
  GimpParam         *values;    /* Return values */
  GimpDrawable      *drawable;  /* Current image */

  /*
   * Initialize parameter data...
   */

  status   = GIMP_PDB_SUCCESS;
  run_mode = param[0].data.d_int32;
  values = g_new (GimpParam, 1);

  *nreturn_vals = 1;
  *return_vals  = values;

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  /*
   * Get drawable information...
   */

  drawable = gimp_drawable_get (param[2].data.d_drawable);
  gimp_tile_cache_ntiles (2 * drawable->ntile_cols);

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
      if (nparams != 10)
        status = GIMP_PDB_CALLING_ERROR;
      else{
        denoise_params.filter = param[3].data.d_float;
	denoise_params.soft   = param[4].data.d_int32;
	denoise_params.multiscale = param[5].data.d_int32;
        denoise_params.f1 = param[6].data.d_float;
        denoise_params.f2 = param[7].data.d_float;
        denoise_params.f3 = param[8].data.d_float;
        denoise_params.f4 = param[9].data.d_float;
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
   * Let the user know what we're doing...
   */
  gimp_progress_init( "Denoising");

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
  denoise_work(w,h,bpp,buffer,1);
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
  if(denoise_params.filter>0. ||
     (denoise_params.multiscale && 
      (denoise_params.f1>0. ||
       denoise_params.f2>0. ||
       denoise_params.f3>0. ||
       denoise_params.f4>0.)))
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
  if(denoise_params.f1>0. ||
     denoise_params.f2>0. ||
     denoise_params.f3>0. ||
     denoise_params.f4>0.)
    gimp_preview_invalidate (GIMP_PREVIEW (preview));
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
                    NULL);

  /* Main filter strength adjust */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              "_Denoise Master", 300, 0,
                              denoise_params.filter,
                              0, 20, .1, 1, 1,
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

  /* multiscale adjust select */
  button = gtk_check_button_new_with_mnemonic ("Multiscale _adjustment");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                denoise_params.multiscale);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_multiscale_callback),
                    NULL);

  /* Subadjustments for autoclean */
  table = gtk_table_new (4, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* fine detail adjust */
  madj[0] = adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 0,
				       "_Very fine denoise:", 300, 0,
				       denoise_params.f1,
				       0, 20, .1, 1, 1,
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
				       0, 20, .1, 1, 1,
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
				       0, 20, .1, 1, 1,
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
                              0, 20, .1, 1, 1,
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

  return run;
}

static void preview_update (GimpPreview *preview)
{
  GimpDrawable *drawable;
  GimpPixelRgn  rgn;            /* previw region */
  gint          x1, y1;
  gint          w,h;
  guchar       *buffer;
  gint          bpp;        /* Bytes-per-pixel in image */


  gimp_preview_get_position (preview, &x1, &y1);
  gimp_preview_get_size (preview, &w, &h);
  drawable = gimp_drawable_preview_get_drawable (GIMP_DRAWABLE_PREVIEW (preview));
  bpp = gimp_drawable_bpp (drawable->drawable_id);
  buffer = g_new (guchar, w * h * bpp);
  gimp_pixel_rgn_init (&rgn, drawable,
                       x1, y1, w, h,
                       FALSE, FALSE);
  gimp_pixel_rgn_get_rect (&rgn, buffer, x1, y1, w, h);
  denoise_work(w,h,bpp,buffer,0);
  gimp_preview_draw_buffer (preview, buffer, w*bpp);
  gimp_drawable_flush (drawable);

  g_free (buffer);
}

static void denoise_work(int width, int height, int planes, guchar *buffer, int pr){
  int i;
  double T[16];

  for(i=0;i<16;i++)
    T[i]=denoise_params.filter;
  if(denoise_params.multiscale){
    T[0]+=denoise_params.f1;
    T[1]+=denoise_params.f2;
    T[2]+=denoise_params.f3;
    for(i=3;i<16;i++)
      T[i]+=denoise_params.f4;
  }

  wavelet_filter(width, height, planes, buffer, pr, T, denoise_params.soft);

}

