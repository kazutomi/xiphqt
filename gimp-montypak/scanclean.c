/*
 *   Text scan cleanup helper for GIMP - The GNU Image Manipulation Program
 *
 *   Copyright 2008 Monty
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
#include <stdlib.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

/*
 * Constants...
 */

#define PLUG_IN_PROC    "plug-in-scanclean"
#define PLUG_IN_BINARY  "scanclean"
#define PLUG_IN_VERSION "23 Nov 2008"

/*
 * Local functions...
 */

static void     query (void);
static void     run   (const gchar      *name,
                       gint              nparams,
                       const GimpParam  *param,
                       gint             *nreturn_vals,
                       GimpParam       **returm_vals);

static void     scanclean_pre    (GimpDrawable *drawable);
static void     scanclean        (GimpDrawable *drawable);
static void     scanclean_work   (int w,int h,int bpp,guchar *buffer, int pr);

static gboolean scanclean_dialog (GimpDrawable *drawable);

static void     preview_update (GimpPreview  *preview);
static void     dialog_autolevel_callback  (GtkWidget     *widget,
                                            gpointer       data);
static void     dialog_autoclean_callback  (GtkWidget     *widget,
					   gpointer       data);
static void     dialog_autorotate_callback  (GtkWidget     *widget,
					     gpointer       data);
static void     dialog_autocrop_callback  (GtkWidget     *widget,
					   gpointer       data);
static void     dialog_scale_callback     (GtkWidget     *widget,
					   gpointer       data);

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
  gint black_percent;
  gint white_percent;

  gint autolevel;

  gint autoclean;
  gint mass;
  gint distance;

  gint autorotate;

  gint autocrop;
  gint binding;

  gint scalen;
  gint scaled;
  gint sharpen;
} ScancleanParams;

static ScancleanParams scanclean_params =
{
  0,100,
  
  1,

  1,6,4,
  
  1,

  1,0,

  1,1,30
};

static GtkWidget    *preview;

MAIN ()

static void
query (void)
{
  static const GimpParamDef   args[] =
  {
    { GIMP_PDB_INT32,    "run-mode",      "Interactive, non-interactive"      },
    { GIMP_PDB_IMAGE,    "image",         "Input image"                       },
    { GIMP_PDB_DRAWABLE, "drawable",      "Input drawable"                    },
    { GIMP_PDB_INT32,    "black_percent", "Black level adjustment" },
    { GIMP_PDB_INT32,    "white_percent", "White level adjustment" },
    { GIMP_PDB_INT32,    "autoclean",     "Clean up splotches/smudges"},
    { GIMP_PDB_INT32,    "mass",          "Autoclean 'mass' threshold adjustment"},
    { GIMP_PDB_INT32,    "distance",      "Autoclean 'distance' threshold adjustment"},
    { GIMP_PDB_INT32,    "autorotate",    "Autorotate text to level"},
    { GIMP_PDB_INT32,    "autocrop",      "Autocrop image down to text"},
    { GIMP_PDB_INT32,    "binding",       "Where autocrop should look for a binding"},
    { GIMP_PDB_INT32,    "scalen",        "Image scaling numerator"},
    { GIMP_PDB_INT32,    "scaled",        "Image scaling denominator"},
    { GIMP_PDB_INT32,    "sharpen",       "Post-scale sharpening amount"}

  };

  gimp_install_procedure (PLUG_IN_PROC,
                          "Clean up scanned text",
			  "This plugin is used to automatically perform "
			  "most of the drudge tasks in cleaning up pages of scanned text. "
			  "Works on selection, deselect non-text.",
                          "Monty <monty@xiph.org>",
                          "Copyright 2008 by Monty",
                          PLUG_IN_VERSION,
                          "S_canclean...",
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

  scanclean_pre(drawable);

  /*
   * See how we will run
   */

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &scanclean_params);

      /*
       * Get information from the dialog...
       */
      if (!scanclean_dialog (drawable))
        return;
      break;

    case GIMP_RUN_NONINTERACTIVE:
      /*
       * Make sure all the arguments are present...
       */
      if (nparams != 15)
        status = GIMP_PDB_CALLING_ERROR;
      else
        scanclean_params.black_percent = param[3].data.d_int32;
        scanclean_params.white_percent = param[4].data.d_int32;
        scanclean_params.autolevel = param[5].data.d_int32;
        scanclean_params.autoclean = param[6].data.d_int32;
        scanclean_params.mass = param[7].data.d_int32;
        scanclean_params.distance = param[8].data.d_int32;
        scanclean_params.autorotate = param[9].data.d_int32;
        scanclean_params.autocrop = param[10].data.d_int32;
        scanclean_params.binding = param[11].data.d_int32;
        scanclean_params.scalen = param[12].data.d_int32;
        scanclean_params.scaled = param[13].data.d_int32;
        scanclean_params.sharpen = param[14].data.d_int32;
      break;

    case GIMP_RUN_WITH_LAST_VALS:
      /*
       * Possibly retrieve data...
       */
      gimp_get_data (PLUG_IN_PROC, &scanclean_params);
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
          scanclean (drawable);

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
                           &scanclean_params, sizeof (ScancleanParams));
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

static void scanclean (GimpDrawable *drawable){

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
  gimp_progress_init( "Cleaning scanned text...");

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
  scanclean_work(w,h,bpp,buffer,1);
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

/*
 * 'scanclean_dialog()' - Popup a dialog window for the filter box size...
 */

static gboolean
scanclean_dialog (GimpDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *button;
  GtkObject *adj;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, TRUE);

  dialog = gimp_dialog_new ("Scanclean", PLUG_IN_BINARY,
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

  /* Black level adjust */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              "_Black level adjust:", 400, 0,
                              scanclean_params.black_percent,
                              0, 100, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &scanclean_params.black_percent);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* White level adjust */
  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  adj = gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                              "_White level adjust:", 400, 0,
                              scanclean_params.white_percent,
                              0, 100, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &scanclean_params.white_percent);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* autolevel select */
  button = gtk_check_button_new_with_mnemonic ("Auto_level");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                scanclean_params.autolevel);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_autolevel_callback),
                    NULL);

  /* autoclean select */
  button = gtk_check_button_new_with_mnemonic ("Auto_clean");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                scanclean_params.autoclean);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_autoclean_callback),
                    NULL);

  /* Subadjustments for autoclean */
  table = gtk_table_new (2, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);


  /* autoclean mass adjust */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 0,
                              "_Mass threshold:", 400, 0,
                              scanclean_params.mass,
                              1, 100, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &scanclean_params.mass);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* autoclean distance adjust */
  adj = gimp_scale_entry_new (GTK_TABLE (table), 1, 1,
                              "_Distance threshold:", 400, 0,
                              scanclean_params.distance,
                              1, 20, 1, 10, 0,
                              TRUE, 0, 0,
                              NULL, NULL);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_int_adjustment_update),
                    &scanclean_params.distance);
  g_signal_connect_swapped (adj, "value-changed",
                            G_CALLBACK (gimp_preview_invalidate),
                            preview);

  /* autorotate select */
  button = gtk_check_button_new_with_mnemonic ("Auto_rotate");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                scanclean_params.autorotate);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_autorotate_callback),
                    NULL);

  /* scale select */
  button = gtk_check_button_new_with_mnemonic ("_Scale 2:1");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                scanclean_params.scaled!=1);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_scale_callback),
                    NULL);

  /* autocrop select */
  button = gtk_check_button_new_with_mnemonic ("Auto_crop");
  gtk_box_pack_start (GTK_BOX (main_vbox), button, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                scanclean_params.autocrop);
  gtk_widget_show (button);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (dialog_autocrop_callback),
                    NULL);

  gtk_widget_show (dialog);

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void dialog_autolevel_callback (GtkWidget *widget,
                          gpointer   data)
{
  scanclean_params.autolevel = (GTK_TOGGLE_BUTTON (widget)->active);
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}


static void dialog_autoclean_callback (GtkWidget *widget,
                          gpointer   data)
{
  scanclean_params.autoclean = (GTK_TOGGLE_BUTTON (widget)->active);
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void dialog_autorotate_callback (GtkWidget *widget,
                          gpointer   data)
{
  scanclean_params.autorotate = (GTK_TOGGLE_BUTTON (widget)->active);
  gimp_preview_invalidate (GIMP_PREVIEW (preview));
}

static void dialog_autocrop_callback (GtkWidget *widget,
                          gpointer   data)
{
  scanclean_params.autocrop = (GTK_TOGGLE_BUTTON (widget)->active);
}


static void dialog_scale_callback (GtkWidget *widget,
                          gpointer   data)
{
  if(GTK_TOGGLE_BUTTON (widget)->active){
    scanclean_params.scalen=1;
    scanclean_params.scaled=2;
  }else{
    scanclean_params.scalen=1;
    scanclean_params.scaled=1;
  }
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
  scanclean_work(w,h,bpp,buffer,0);
  gimp_preview_draw_buffer (preview, buffer, w*bpp);
  gimp_drawable_flush (drawable);

  g_free (buffer);
}

static void scanclean_pre (GimpDrawable *drawable){
  GimpPixelRgn  src_rgn;        /* Source image region */
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
  gimp_progress_init( "");

  /*
   * Setup for filter...
   */

  //gimp_pixel_rgn_init (&src_rgn, drawable,
  //                     x1, y1, w, h, FALSE, FALSE);
//buffer = g_new (guchar, w * h * bpp);
//gimp_pixel_rgn_get_rect (&src_rgn, buffer, x1, y1, w, h);

  /* precomputation work */




  /* done */
                      
  // g_free(buffer);
}

#include <stdio.h>
#define sq(x) ((int)(x)*(int)(x))
#define clamp(x) ((x)<0?0:((x)>255?255:(x)))

static int collect_add_row(guchar *b, int *s, int *ss, int w, int h, int row, int x1, int x2){
  int i,j;
  if(row<0||row>=h)return 0;
  if(x1<0)x1=0;
  if(x2>w)x2=w;
  i=row*w+x1;
  j=row*w+x2;
  while(i<j){
    *s+=b[i];
    *ss+=b[i]*b[i];
    i++;
  }
  return(x2-x1);
}

static int collect_sub_row(guchar *b, int *s, int *ss, int w, int h, int row, int x1, int x2){
  int i,j;
  if(row<0||row>=h)return 0;
  if(x1<0)x1=0;
  if(x2>w)x2=w;
  i=row*w+x1;
  j=row*w+x2;
  while(i<j){
    *s-=b[i];
    *ss-=b[i]*b[i];
    i++;
  }
  return(x1-x2);
}

static int collect_add_col(guchar *b, int *s, int *ss, int w, int h, int col, int y1, int y2){
  int i,j;
  if(col<0||col>=w)return 0;
  if(y1<0)y1=0;
  if(y2>h)y2=h;
  i=y1*w+col;
  j=y2*w+col;
  while(i<j){
    *s+=b[i];
    *ss+=b[i]*b[i];
    i+=w;
  }
  return(y2-y1);
}

static int collect_sub_col(guchar *b, int *s, int *ss, int w, int h, int col, int y1, int y2){
  int i,j;
  if(col<0||col>=w)return 0;
  if(y1<0)y1=0;
  if(y2>h)y2=h;
  i=y1*w+col;
  j=y2*w+col;
  while(i<j){
    *s-=b[i];
    *ss-=b[i]*b[i];
    i+=w;
  }
  return(y1-y2);
}

#include "blur.c"

/* sliding window mean/variance collection */
#if 0
static inline void collect_var(guchar *b, double *v, guchar *m, int w, int h, int n){
  int i;
  memcpy(m,b,sizeof(*b)*w*h);
  blur(m,w,h,n);

  for(i=0;i<w*h;i++)
    v[i] = (double)b[i]*b[i];
  blurd(v,w,h,n);

  for(i=0;i<w*h;i++)
    v[i] -= (double)m[i]*m[i]; 

}
#else

static inline void collect_var(guchar *b, double *v, guchar *m, int w, int h, int n){
  int x,y;
  int sum=0;
  int ssum=0;
  int acc=0;
  double d = 1;

  /* prime */
  for(y=0;y<=n;y++)
    acc+=collect_add_row(b,&sum,&ssum,w,h,y,0,n+1);
  d=1./acc;

  for(x=0;x<w;){
    /* even x == increasing y */

    m[x] = sum*d;
    v[x] = ssum*d - m[x]*m[x];

    for(y=1;y<h;y++){
      int nn=collect_add_row(b,&sum,&ssum,w,h,y+n,x-n,x+n+1) +
	collect_sub_row(b,&sum,&ssum,w,h,y-n-1,x-n,x+n+1);
      if(nn){
	acc += nn;
	d = 1./acc;
      }
      
      m[y*w+x] = sum*d;
      v[y*w+x] = ssum*d - m[y*w+x]*m[y*w+x];

    }

    x++;
    if(x>=w)break;

    acc+=collect_add_col(b,&sum,&ssum,w,h,x+n,h-n-1,h);
    acc+=collect_sub_col(b,&sum,&ssum,w,h,x-n-1,h-n-1,h);
    d=1./acc;

    /* odd x == decreasing y */
    m[(h-1)*w+x] = sum*d;
    v[(h-1)*w+x] = ssum*d - m[(h-1)*w+x]*m[(h-1)*w+x];

    for(y=h-2;y>=0;y--){
      int nn=collect_add_row(b,&sum,&ssum,w,h,y-n,x-n,x+n+1) +
	collect_sub_row(b,&sum,&ssum,w,h,y+n+1,x-n,x+n+1);
      if(nn){
	acc += nn;
	d = 1./acc;
      }
      
      m[y*w+x] = sum*d;
      v[y*w+x] = ssum*d - m[y*w+x]*m[y*w+x];
    }
    x++;
    acc+=collect_add_col(b,&sum,&ssum,w,h,x+n,0,n+1);
    acc+=collect_sub_col(b,&sum,&ssum,w,h,x-n-1,0,n+1);
    d=1./acc;

  }

}
#endif
static inline void metrics(guchar *b, float *v, float *m, int w, int n){
  int i,j;
  int sum=0,ssum=0;
  float d = 1./sq(n*2+1);
  b-=w*n;

  for(i=-n;i<=n;i++){
    for(j=-n;j<=n;j++){
      sum += *(b+j);
      ssum += *(b+j) * *(b+j);
    }
    b+=w;
  }

  *m = sum * d;
  *v = ssum*d - *m**m;
}

static inline void bound_metrics(guchar *b, float *v, float *m, int w, int h, int x, int y, int nn){
  int i,j;
  int sum=0,ssum=0,n=0;

  for(i=y-nn;i<=y+nn;i++){
    if(i>=0 && i<h){ 
      for(j=x-nn;j<=x+nn;j++){
	if(j>=0 && j<w){ 
	  sum += b[i*w+j];
	  ssum += (int)b[i*w+j]*b[i*w+j];
	  n++;
	}
      }
    }
  }

  m[y*w+x] = (float)sum/n;
  v[y*w+x] = (float)ssum/n - m[y*w+x]*m[y*w+x];
}

static int cmpint(const void *p1, const void *p2){
  int a = *(int *)p1;
  int b = *(int *)p2;
  return a-b;
}

static void background_heal(guchar *bp,guchar* fp,int w,int h,int rn){
  int x,y;
  for(y=0;y<h;y++)
    for(x=0;x<w;x++)
      if(fp[y*w+x]){
	int i;
	int j;
	int acc=0,nn=0;
	int b=0,bn=0;
	int n;
	for(n=1;(n<rn || nn<20) && n<50;n++){
	  j = x-n;
	  if(j>=0)
	    for(i=y-n+1;i<y+n-1;i++)
	      if(i>=0 && i<h){
		if(!fp[i*w+j]){
		  acc+=bp[i*w+j];
		  nn++;
		}
	      }
	  
	  j = x+n;
	  if(j<w)
	    for(i=y-n+1;i<y+n-1;i++)
	      if(i>=0 && i<h){
		if(!fp[i*w+j]){
		  acc+=bp[i*w+j];
		  nn++;
		}
	      }
	  
	  i = y-n;
	  if(i>=0)
	    for(j=x-n;j<x+n;j++)
	      if(j>=0 && j<w){
		if(!fp[i*w+j]){
		  acc+=bp[i*w+j];
		  nn++;
		}
	      }
	  
	  i = y+n;
	  if(i<h)
	    for(j=x-n;j<x+n;j++)
	      if(j>=0 && j<w){
		if(!fp[i*w+j]){
		  acc+=bp[i*w+j];
		  nn++;  
		}
	      }
	}
	if(nn)
	  bp[y*w+x]= ROUND((float)acc/nn);
	else
	  bp[y*w+x]= 255;
      }
}

static int flood_recurse(guchar *f, int w,int h,int x,int y,int d){
  if(d<10000){
    int loop=0;
    if(y>=0 && y<h && x>=0 && x < w){
      if(f[y*w+x]==1){
	int x1=x, x2=x+1;
	f[y*w+x]=2;
	while(x1>0 && f[y*w+x1-1]==1){
	  x1--;
	  f[y*w+x1]=2;
	}
	while(x2<w && f[y*w+x2]==1){
	  f[y*w+x2]=2;
	  x2++;
	}
	for(x=x1;x<x2;x++){
	  int y1=y,y2=y+1,yy;

	  while(y1>0 && f[(y1-1)*w+x]==1){
	    y1--;
	    f[y1*w+x]=2;
	  }
	  while(y2<h && f[y2*w+x]==1){
	    f[y2*w+x]=2;
	    y2++;
	  }

	  for(yy=y1;yy<y2;yy++){
	    loop |= flood_recurse(f,w,h,x-1,yy,d+1);
	    loop |= flood_recurse(f,w,h,x+1,yy,d+1);
	  }

	}
      }
    }
    return loop;
  }else
    return 1;
}

static void flood_foreground(guchar *f, int w, int h){
  int x,y,loop=1;
  while(loop){
    loop=0;
    for(y=0;y<h;y++)
      for(x=0;x<w;x++)
	if(f[y*w+x]==3){
	  loop |= flood_recurse(f,w,h,x,y-1,0);
	  loop |= flood_recurse(f,w,h,x-1,y,0);
	  loop |= flood_recurse(f,w,h,x,y+1,0);
	  loop |= flood_recurse(f,w,h,x+1,y,0);
	}
  }
  
  for(y=0;y<h;y++)
    for(x=0;x<w;x++)
      if(f[y*w+x]<2)
	f[y*w+x]=0;
}

static void median_filter(guchar *d, guchar *o,int w, int h){
  int x,y;

  for(y=1;y<h-1;y++){
    for(x=1;x<w-1;x++){
      int a[8];
      int i = y*w+x,j,k;
      int v = 255; 
      a[0]=d[i-1];
      a[1]=d[i+w-1];
      a[2]=d[i+w];
      a[3]=d[i+w+1];
      a[4]=d[i+1];
      a[5]=d[i-w+1];
      a[6]=d[i-w];
      a[7]=d[i-w-1];

      for(j=0;j<7;j++){
	int ld = 255;
	for(k=0;k<3;k++){
	  int ii = (j+k+3<8?j+k+3:j+k-5);
	  if(a[ii]<ld)ld=a[ii];
	}
	if(ld<a[j])ld=a[j];
	ld=clamp(255-((255-ld)*2));
	if(ld<v)v=ld;
      }

      if(d[i]>=v){
	o[i]=d[i];
	continue;
      }

      {
	int d0=255,d1=255;
	for(j=0;j<7;j++)
	  if(a[j]<d0){
	    d1=d0;
	    d0=a[j];
	  }else 
	    if(a[j]<d1)d1=a[j];
	
	if(d[i]<d1)
	  o[i]=d1;
	else
	  o[i]=d[i];
      }
    }
  }
}

static void grow_foreground(guchar *foreground, int w, int h){
  // horizontal
  int y,x;
  for(y=0;y<h;y++){  
    guchar *p = foreground+w*y;
    
    if(p[1]>1 && !p[0])p[0]=1;
    for(x=0;x<w-2;x++){
      if(p[x]>1){
	if(!p[x+1])p[x+1]=1;
	if(!p[x+2])p[x+2]=1;
      }
      if(p[x+2]>1){
	if(!p[x+1])p[x+1]=1;
	if(!p[x])p[x]=1;
      }
    }
    if(p[x]>1 && !p[x+1])p[x+1]=1;
  }
  
  // vertical 
  {
    guchar *p = foreground;
    guchar *q = foreground+w;
    for(x=0;x<w;x++)
      if(q[x]>1 && !p[x])p[x]=1;
  }
  for(y=0;y<h-2;y++){  
    guchar *p = foreground+w*y;
    guchar *q = foreground+w*(y+1);
    guchar *r = foreground+w*(y+2);
    for(x=0;x<w;x++){
      if(p[x]>1){
	if(!q[x])q[x]=1;
	if(!r[x])r[x]=1;
      }
      if(r[x]>1){
	if(!p[x])p[x]=1;
	if(!q[x])q[x]=1;
      }
    }
  }
  {
    guchar *p = foreground+w*y;
    guchar *q = foreground+w*(y+1);
    for(x=0;x<w;x++)
      if(p[x]>1 && !q[x])q[x]=1;
  }
}

static void scanclean_work(int w, int h, int planes, guchar *buffer, int pr){
  int i,x,y;
  guchar *foreground = g_new(guchar,w*h);
  guchar *filter = g_new(guchar,w*h);
  guchar *background = g_new(guchar,w*h);
  double *variances = g_new(double,w*h);
  guchar *means = g_new(guchar,w*h);
  double *variances2 = g_new(double,w*h);
  guchar *means2 = g_new(guchar,w*h);
  ScancleanParams p = scanclean_params;

  for(i=0;i<planes;i++){
    double d=0.;
    memcpy(filter,buffer,sizeof(*buffer)*w*h);

    /* var/mean for Niblack eqs. */
    collect_var(buffer,variances,means,w,h,20);

    /* Sauvola thresholding for background construction */
    for(i=0;i<w*h;i++){
      foreground[i]=0;
      if(filter[i] < means[i]*(1+.1*(sqrt(variances[i])/128-1.)))foreground[i]=2;
    }

    /* apply spreading function to means */

    /* grow the outer foreground by two */
    grow_foreground(foreground,w,h);

    /* build background  */
    memcpy(background,filter,sizeof(*background)*w*h);
    background_heal(background,foreground,w,h,5);

    /* re-threshold */
    /* Subtly different from the Sauvola method above; although the
       equation looks the same, our threshold is being based on the
       constructed background, *not* the means. */

    for(i=0;i<w*h;i++){
      foreground[i]=0;
      if(filter[i] < background[i]*(1+.5*(sqrt(variances[i])/128-1.)))
	foreground[i]=3;
    }


    for(i=0;i<w*h;i++){
      if(!foreground[i])
	if(filter[i] < background[i]*(1+.15*(sqrt(variances[i])/128-1.)))
	  foreground[i]=1;
      
    }

    


    /* flood fill 'sensitive' areas from 'insensitive' areas */
    flood_foreground(foreground, w, h);

    /* grow the outer foreground by two */
    //grow_foreground(foreground,w,h);

    {
      /* compute global distance */

      int dn=0;
      for(i=0;i<w*h;i++)
	if(foreground[i]==3){
	  d+=means[i]-filter[i];
	  dn++;
	}
      
      d/=dn;
    }

    if(p.autoclean)
    {
      /* The immediate vicinity of a character needs a slight unsharp
	 mask effect or deep stroke indents will be swallowed by black
	 in heavier glyphs, like a Times lowercase 'g'.  This
	 wouldn't be a problem on an ideal scanner, but scanners
	 aren't ideal and some are worse than others. */
      guchar * blurbuf = g_new(guchar,w*h);
      guchar * blurbuf2 = g_new(guchar,w*h);
      memcpy (blurbuf,filter,sizeof(*blurbuf)*w*h);
      memcpy (blurbuf2,filter,sizeof(*blurbuf2)*w*h);

      blur(blurbuf,w,h,.5);
      for(i=0;i<w*h;i++)
	if(foreground[i]){
	  int del = (blurbuf2[i]-blurbuf[i]);
	  means[i]=clamp(means[i]-del);
	}

      g_free(blurbuf);
      g_free(blurbuf2);
    }


    

    

    {      
      //for(i=0;i<w*h;i++){
      //double white = background[i]-(1.-p.white_percent*.01)*d;
      //double black = white - d + (p.black_percent*.01)*d;
      //double dd = white-black;
      //int val = (buffer[i]-black)/dd*255.;
      //buffer[i] = clamp(val);
      //}

      for(i=0;i<w*h;i++){
	if(foreground[i]){
	  double white = means[i]-d*(1.-(p.white_percent*.01+.5));
	  double black = means[i]-d + (p.black_percent*.01)*(means[i]-d);
	  double dd = white-black;
	  int val = (filter[i]-black)/dd*255.;
	  filter[i] = clamp(val);
	}else
	  filter[i] = 255.;
      }


    }



    if(p.autorotate)
      for(i=0;i<w*h;i++)
	buffer[i]=sqrt(variances[i]);
    else
      memcpy(buffer,filter,sizeof(guchar)*w*h);


  }

  if(p.autolevel)
    memcpy(buffer,means,sizeof(guchar)*w*h);
  
  g_free(foreground);
  g_free(background);
  g_free(filter);
  g_free(variances);
  g_free(means);
  g_free(variances2);
  g_free(means2);
}
