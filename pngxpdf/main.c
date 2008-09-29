/*
 *
 *     pngxpdf copyright (C) 2008 Monty <monty@xiph.org>
 *
 *  pngxpdf is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  pngxpdf is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with pngxpdf; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-pdf.h>

const char *optstring = "W:H:r:t:mhvx:y:f:";

struct option options [] = {
  {"width",required_argument,NULL,'W'},
  {"height",required_argument,NULL,'H'},
  {"resolution",required_argument,NULL,'r'},
  {"text",required_argument,NULL,'t'},
  {"help",no_argument,NULL,'h'},
  {"version",no_argument,NULL,'v'},
  {"x-offset",required_argument,NULL,'x'},
  {"y-offset",required_argument,NULL,'y'},
  {"font-size",required_argument,NULL,'f'},

  {NULL,0,NULL,0}
};

cairo_status_t pdf_write(void *closure,
			 const unsigned char *data,
			 unsigned int length){
  if(fwrite(data, 1, length, stdout)<length)
    return CAIRO_STATUS_WRITE_ERROR;
  return CAIRO_STATUS_SUCCESS;
}

void usage(FILE *f){
  fprintf(f,"pngxpdf "VERSION"\n"
	  "(C) 2008 Monty <monty@xiph.org>\n\n"
	  "USAGE:\n\n"
	  "  pnxpdf [options] file1.png [file2.png...] > output.pdf\n\n"
	  "OPTIONS:\n\n"
	  "  -h --help           : Display this usage information\n\n"
	  "  -f --font-size <n>  : Font size (points)\n"
	  "  -H --height <n>     : Height of each output page; suffix with mm, cm,\n"
	  "                        in or pt to use various units. default is 11.0in\n\n"
	  "  -r --resolution <n> : Specify input resolution of each image.  Each\n"
	  "                        image is centered on the page and either cropped\n"
	  "                        to fit or placed within a border.  Suffix with\n"
	  "                        dpi, dpcm, dpmm, or dpp to specify different\n"
	  "                        units.  150dpi default.\n\n"
	  "  -t --text <string>  : string comment to add as a footer to each page\n\n"
	  "  -v --version        : Output version string and exit\n\n"
	  "  -W --width <n>      : Width of each output page; suffix with mm, cm,\n"
	  "                        in or pt to use various units. default is 8.5in\n"
	  "  -x --x-offset <n>   : Left or right offset of image center; suffix with\n"
	  "                        mm, cm, in or pt to use various units.\n\n"
	  "  -y --y-offset <n>   : Up or down offset of image center; suffix with\n"
	  "                        mm, cm, in or pt to use various units. Use -y 0\n"
	  "                        to eliminate text footer from shifting default\n"
	  "                        center upwards.\n\n");
}

int main(int argc, char **argv){
  float width=8.5*72.0;
  float height=11.0*72.0;
  float xoff=0;
  float yoff=0;
  float fontsize=-1;
  int havey=0;
  float dpp=300.0/72.0;
  char *text=NULL;

  int c,long_option_index;

  cairo_surface_t *cs;
  cairo_t *ct;
  cairo_text_extents_t extents;

  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'W':
    case 'H':
    case 'x':
    case 'y':
      {
	float temp;
	if(strstr(optarg,"cm")){
	  temp=atof(optarg)*28.3464566929;
	}else if (strstr(optarg,"mm")){
	  temp=atof(optarg)*2.83464566929;
	}else if (strstr(optarg,"pt")){
	  temp=atof(optarg);
	}else{
	  temp=atof(optarg)*72.0;
	}
	switch(c){
	case 'W':
	  width=temp;
	  break;
	case 'H':
	  height=temp;
	  break;
	case 'x':
	  xoff=temp;
	  break;
	case 'y':
	  yoff=temp;
	  havey=1;
	  break;
	}
      }
      break;
    case 'r':
      if(strstr(optarg,"dpcm")){
	dpp=atof(optarg)*.03527777777778;
      }else if (strstr(optarg,"dpmm")){
	dpp=atof(optarg)*.35277777777778;
      }else if (strstr(optarg,"dpp")){
	dpp=atof(optarg);
      }else{
	dpp=atof(optarg)*.01388888888889;
      }
      break;
    case 'f':
      fontsize=atof(optarg);
      break;
    case 't':
      text=strdup(optarg);
      break;
    case 'h':
      usage(stdout);
      exit(0);
    case 'v':
      fprintf(stderr,"pngxpdf "VERSION"\n");
    default:
      usage(stderr);
    }
  }

  /* set up our surface */
  cs = cairo_pdf_surface_create_for_stream (pdf_write, NULL, width, height);
  if(!cs || cairo_surface_status(cs)!=CAIRO_STATUS_SUCCESS){
    fprintf(stderr,"CAIRO ERROR: Unable to create PDF surface.\n\n");
    exit(1);
  }
  ct = cairo_create(cs);
  if(fontsize<=0){
    fontsize=height*15./792.;
    if(fontsize<5)fontsize=5;
  }
  cairo_set_font_size(ct, fontsize);
  if(text){
    cairo_text_extents(ct, text, &extents);
    if(!havey)
      yoff = -extents.height-fontsize*4;
  }
  
  /* Iterate through PNG files inline */
  while(optind<argc){
    int ww, hh;
    char *filename = argv[optind];
    cairo_pattern_t *pattern;
    cairo_surface_t *ps=cairo_image_surface_create_from_png(filename);
    cairo_status_t status = cairo_surface_status(ps);
    if(!ps || status!=CAIRO_STATUS_SUCCESS){
      fprintf(stderr,"CAIRO ERROR: Unable to load PNG file %s: %s\n\n",filename,cairo_status_to_string(status));
      exit(1);
    }
    ww = cairo_image_surface_get_width(ps);
    hh = cairo_image_surface_get_height(ps);

    cairo_save(ct);
    cairo_scale(ct, 1./dpp, 1./dpp);
    pattern = cairo_pattern_create_for_surface(ps);
    cairo_translate(ct,(width*dpp - (ww-1))*.5,((height+yoff)*dpp - (hh-1))*.5);
    cairo_pattern_set_filter(pattern, CAIRO_FILTER_BEST);
    cairo_set_source(ct,pattern);
    cairo_paint(ct);
    cairo_restore(ct);

    /* draw comment text */
    if(text){
      cairo_set_source_rgba(ct, 1,1,1,.75);
      cairo_move_to(ct, width-extents.width-fontsize*1.5, height-fontsize*1.5);
      cairo_text_path (ct, text);  
      cairo_set_line_width(ct,3.);
      cairo_set_line_join(ct,CAIRO_LINE_JOIN_ROUND);
      cairo_stroke(ct);

      cairo_set_source_rgb(ct, 0,0,0);
      cairo_move_to(ct, width-extents.width-fontsize*1.5, height-fontsize*1.5);
      cairo_show_text(ct, text);  

    }


    cairo_surface_show_page(cs);


    cairo_surface_destroy(ps);
    optind++;
  }

  cairo_destroy(ct);
  cairo_surface_destroy(cs);
}
