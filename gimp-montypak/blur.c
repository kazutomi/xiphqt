/*
 * blur.c
 * Taken from unsharp.c, originally:
 * Copyright (C) 1999 Winston Chang
 *                    <winstonc@cs.wisc.edu>
 *                    <winston@stdout.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>

static gint      gen_convolve_matrix (gdouble         std_dev,
                                      gdouble       **cmatrix);
static gdouble * gen_lookup_table    (const gdouble  *cmatrix,
                                      gint            cmatrix_length);
static void      unsharp_region      (GimpPixelRgn   *srcPTR,
                                      GimpPixelRgn   *dstPTR,
                                      gint            bytes,
                                      gdouble         radius,
                                      gdouble         amount,
                                      gint            x1,
                                      gint            x2,
                                      gint            y1,
                                      gint            y2,
                                      gboolean        show_progress);

static void      unsharp_mask        (GimpDrawable   *drawable,
                                      gdouble         radius,
                                      gdouble         amount);

static gboolean  unsharp_mask_dialog (GimpDrawable   *drawable);
static void      preview_update      (GimpPreview    *preview);


/* This function is written as if it is blurring a column at a time,
 * even though it can operate on rows, too.  There is no difference
 * in the processing of the lines, at least to the blur_line function.
 */
static void
blur_line (const gdouble *cmatrix,
           const gint     cmatrix_length,
           const guchar  *src,
           guchar        *dest,
           const gint     len){
  const gdouble *cmatrix_p;
  const guchar  *src_p;
  const gint     cmatrix_middle = cmatrix_length / 2;
  gint           row;
  gint           i, j;

  /* This first block is the same as the optimized version --
   * it is only used for very small pictures, so speed isn't a
   * big concern.
   */
  if (cmatrix_length > len) {
    for (row = 0; row < len; row++){
      /* find the scale factor */
      gdouble scale = 0;
      gdouble sum = 0;

      for (j = 0; j < len; j++){
	/* if the index is in bounds, add it to the scale counter */
	if (j + cmatrix_middle - row >= 0 &&
	    j + cmatrix_middle - row < cmatrix_length)
	  scale += cmatrix[j];
      }

      src_p = src;
      for (j = 0; j < len; j++){
	if (j + cmatrix_middle - row >= 0 &&
	    j + cmatrix_middle - row < cmatrix_length)
	  sum += *src_p++ * cmatrix[j];
      }
	  
      *dest++ = (guchar) ROUND (sum / scale);
    }
  }else{
    /* for the edge condition, we only use available info and scale to one */
    for (row = 0; row < cmatrix_middle; row++){
      /* find scale factor */
      gdouble scale = 0;
      gdouble sum = 0;
      
      for (j = cmatrix_middle - row; j < cmatrix_length; j++)
	scale += cmatrix[j];
      
      src_p = src;
      for (j = cmatrix_middle - row; j < cmatrix_length; j++)
	sum += *src_p++ * cmatrix[j];
      
      *dest++ = (guchar) ROUND (sum / scale);
    }

    /* go through each pixel in each col */
    for (; row < len - cmatrix_middle; row++){
      gdouble sum = 0;
      src_p = src++;
	    
      for (j = 0; j < cmatrix_length; j++)
	sum += *src_p++ * cmatrix[j];

      *dest++ = (guchar) ROUND (sum);
    }

    /* for the edge condition, we only use available info and scale to one */
    for (; row < len; row++){
      /* find scale factor */
      gdouble scale = 0;
      gdouble sum = 0;
      
      for (j = 0; j < len - row + cmatrix_middle; j++)
	scale += cmatrix[j];

      src_p = src++;
      for (j = 0; j < len - row + cmatrix_middle; j++)
	sum += *src_p++ * cmatrix[j];

      *dest++ = (guchar) ROUND (sum / scale);
    }
  }
}

static void
blur_lined (const gdouble *cmatrix,
           const gint     cmatrix_length,
           const double  *src,
           double        *dest,
           const gint     len){
  const gdouble *cmatrix_p;
  const double  *src_p;
  const gint     cmatrix_middle = cmatrix_length / 2;
  gint           row;
  gint           i, j;

  /* This first block is the same as the optimized version --
   * it is only used for very small pictures, so speed isn't a
   * big concern.
   */
  if (cmatrix_length > len) {
    for (row = 0; row < len; row++){
      /* find the scale factor */
      gdouble scale = 0;
      gdouble sum = 0;

      for (j = 0; j < len; j++){
	/* if the index is in bounds, add it to the scale counter */
	if (j + cmatrix_middle - row >= 0 &&
	    j + cmatrix_middle - row < cmatrix_length)
	  scale += cmatrix[j];
      }

      src_p = src;
      for (j = 0; j < len; j++){
	if (j + cmatrix_middle - row >= 0 &&
	    j + cmatrix_middle - row < cmatrix_length)
	  sum += *src_p++ * cmatrix[j];
      }
	  
      *dest++ = sum / scale;
    }
  }else{
    /* for the edge condition, we only use available info and scale to one */
    for (row = 0; row < cmatrix_middle; row++){
      /* find scale factor */
      gdouble scale = 0;
      gdouble sum = 0;
      
      for (j = cmatrix_middle - row; j < cmatrix_length; j++)
	scale += cmatrix[j];
      
      src_p = src;
      for (j = cmatrix_middle - row; j < cmatrix_length; j++)
	sum += *src_p++ * cmatrix[j];
      
      *dest++ = sum / scale;
    }

    /* go through each pixel in each col */
    for (; row < len - cmatrix_middle; row++){
      gdouble sum = 0;
      src_p = src++;
	    
      for (j = 0; j < cmatrix_length; j++)
	sum += *src_p++ * cmatrix[j];

      *dest++ = sum;
    }

    /* for the edge condition, we only use available info and scale to one */
    for (; row < len; row++){
      /* find scale factor */
      gdouble scale = 0;
      gdouble sum = 0;
      
      for (j = 0; j < len - row + cmatrix_middle; j++)
	scale += cmatrix[j];

      src_p = src++;
      for (j = 0; j < len - row + cmatrix_middle; j++)
	sum += *src_p++ * cmatrix[j];

      *dest++ = sum / scale;
    }
  }
}

static void blur (guchar *b,
		  int     w,
		  int     h,
		  double  radius){

  gdouble *cmatrix = NULL;
  gint     cmatrix_length = gen_convolve_matrix (radius, &cmatrix);

  /* allocate buffers */
  guchar *src  = g_new (guchar, w+h);
  guchar *dest = g_new (guchar, w+h);
  int x,y;


  /* blur the rows */
  for (y = 0; y < h; y++){
    memcpy(src,b+y*w,sizeof(*b)*w);
    blur_line (cmatrix, cmatrix_length, src, dest, w);
    memcpy(b+y*w,dest,sizeof(*b)*w);
  }

  /* blur the cols */
  for (x = 0; x < w; x++){
    for(y = 0; y < h; y++) src[y]=b[y*w+x];
    blur_line (cmatrix, cmatrix_length, src, dest, h);
    for(y = 0; y < h; y++) b[y*w+x]=dest[y];
  }

  g_free (dest);
  g_free (src);
  g_free (cmatrix);
}

static void blurd (double *b,
		   int     w,
		   int     h,
		   double  radius){
  
  gdouble *cmatrix = NULL;
  gint     cmatrix_length = gen_convolve_matrix (radius, &cmatrix);

  /* allocate buffers */
  double *src  = g_new (double, w+h);
  double *dest = g_new (double, w+h);
  int x,y;


  /* blur the rows */
  for (y = 0; y < h; y++){
    memcpy(src,b+y*w,sizeof(*b)*w);
    blur_lined (cmatrix, cmatrix_length, src, dest, w);
    memcpy(b+y*w,dest,sizeof(*b)*w);
  }

  /* blur the cols */
  for (x = 0; x < w; x++){
    for(y = 0; y < h; y++) src[y]=b[y*w+x];
    blur_lined (cmatrix, cmatrix_length, src, dest, h);
    for(y = 0; y < h; y++) b[y*w+x]=dest[y];
  }

  g_free (dest);
  g_free (src);
  g_free (cmatrix);
}

/* generates a 1-D convolution matrix to be used for each pass of
 * a two-pass gaussian blur.  Returns the length of the matrix.
 */
static gint
gen_convolve_matrix (gdouble   radius,
                     gdouble **cmatrix_p)
{
  gdouble *cmatrix;
  gdouble  std_dev;
  gdouble  sum;
  gint     matrix_length;
  gint     i, j;

  /* we want to generate a matrix that goes out a certain radius
   * from the center, so we have to go out ceil(rad-0.5) pixels,
   * inlcuding the center pixel.  Of course, that's only in one direction,
   * so we have to go the same amount in the other direction, but not count
   * the center pixel again.  So we double the previous result and subtract
   * one.
   * The radius parameter that is passed to this function is used as
   * the standard deviation, and the radius of effect is the
   * standard deviation * 2.  It's a little confusing.
   */
  radius = fabs (radius) + 1.0;

  std_dev = radius;
  radius = std_dev * 2;

  /* go out 'radius' in each direction */
  matrix_length = 2 * ceil (radius - 0.5) + 1;
  if (matrix_length <= 0)
    matrix_length = 1;

  *cmatrix_p = g_new (gdouble, matrix_length);
  cmatrix = *cmatrix_p;

  /*  Now we fill the matrix by doing a numeric integration approximation
   * from -2*std_dev to 2*std_dev, sampling 50 points per pixel.
   * We do the bottom half, mirror it to the top half, then compute the
   * center point.  Otherwise asymmetric quantization errors will occur.
   *  The formula to integrate is e^-(x^2/2s^2).
   */

  /* first we do the top (right) half of matrix */
  for (i = matrix_length / 2 + 1; i < matrix_length; i++)
    {
      gdouble base_x = i - (matrix_length / 2) - 0.5;

      sum = 0;
      for (j = 1; j <= 50; j++)
        {
          gdouble r = base_x + 0.02 * j;

          if (r <= radius)
            sum += exp (- SQR (r) / (2 * SQR (std_dev)));
        }

      cmatrix[i] = sum / 50;
    }

  /* mirror the thing to the bottom half */
  for (i = 0; i <= matrix_length / 2; i++)
    cmatrix[i] = cmatrix[matrix_length - 1 - i];

  /* find center val -- calculate an odd number of quanta to make it symmetric,
   * even if the center point is weighted slightly higher than others. */
  sum = 0;
  for (j = 0; j <= 50; j++)
    sum += exp (- SQR (- 0.5 + 0.02 * j) / (2 * SQR (std_dev)));

  cmatrix[matrix_length / 2] = sum / 51;

  /* normalize the distribution by scaling the total sum to one */
  sum = 0;
  for (i = 0; i < matrix_length; i++)
    sum += cmatrix[i];

  for (i = 0; i < matrix_length; i++)
    cmatrix[i] = cmatrix[i] / sum;

  return matrix_length;
}

