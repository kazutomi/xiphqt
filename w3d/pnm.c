/**
 *   Thanks to Simon who contributed the code to make these functions 
 *   CR/LF safe ! 
 */

#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "mem.h"



static
int eat_ws_comments(FILE *fp)
{
   int c;

   while ((c = fgetc(fp)) != EOF) {
      if (c == '#') {                    /* eat the comment */
         do {
            c = fgetc(fp);
            if (c == EOF)                /* but bail if we hit EOF */
               return -1;
         } while (c != '\n');
      }

      if (!isspace(c)) {                 /* we are done */
         return ungetc(c,fp);
      }
   }

   return -1;  /* fell out of the loop */
}


/* this is a kludge, but raw pnm's are not well defined... */
static
int eat_ws_to_eol (FILE *fp)
{
   int c;

   do {
      c = fgetc(fp);

      if (c == '\n' || c == '\r') /* some dos/win pnms generated with */
         return c;                /* just '\r' after maxval */
   } while (isspace(c));

   return ungetc(c,fp);
}


static
int read_pnm_header_internal (FILE *fp, int *w, int *h)
{
   int type, maxval;

   if (fgetc(fp) != 'P')
      return -1;

   type = fgetc(fp) - '0';

   if (type < 0 || type > 6)
      return -1;               /* invalid type */

   eat_ws_comments(fp);
   fscanf(fp, "%d", w);

   eat_ws_comments(fp);
   fscanf(fp, "%d", h);

   eat_ws_comments(fp);
   fscanf(fp, "%d", &maxval);
   eat_ws_to_eol(fp);

   return type;
}


int read_pnm_header (char *fname, int *w, int *h)
{
   int type;
   FILE *fp = fopen(fname, "r");

   if (!fp)
      return -1;

   type = read_pnm_header_internal (fp, w, h);

   fclose (fp);

   return (type == 3 || type == 6) ? 3 : 1;
}


int read_pnm (char *fname, uint8_t *buf)
{
   FILE *fp = fopen(fname, "r");
   int type;
   int w, h;

   if (!fp)
      return -1;

   type = read_pnm_header_internal (fp, &w, &h);

   switch (type) {
   case 5:
      if (fread (buf, 1, w*h, fp) != w*h)
         return -1;
      break;
   case 6:
      if (fread (buf, 3, w*h, fp) != w*h)
         return -1;
      break;
   default:
      fprintf (stderr, "%s: unimplemented image format !!!\n", __FUNCTION__);
   };

   fclose (fp);

   return 0;
}


void write_pnm (char *fname, uint8_t *rgb, int w, int h)
{
   FILE *outfile;

   outfile = fopen (fname, "w");

   if (!outfile) {
      fprintf (stderr, "error opening '%s' for writing !!!\n", fname);
      exit (-1);
   }


   if (strstr(fname, ".ppm") == fname + strlen(fname)-4) {
      fprintf (outfile, "P6\n%d %d\n%d\n", w, h, 255);
      fwrite (rgb, 3, w*h, outfile);
   } else if (strstr(fname, ".pgm") == fname + strlen(fname)-4) {
      fprintf (outfile, "P5\n%d %d\n%d\n", w, h, 255);
      fwrite (rgb, 1, w*h, outfile);
   } else {
      fprintf (stderr, 
               "%s: can't write anything else than .ppm's and .pgm's !\n",
               __FUNCTION__);
      exit (-1);
   }

   fclose (outfile);
}


static inline
uint8_t CLAMP(int16_t x)
{
   x *= 4;
   x += 128;
   return  ((x > 255) ? 255 : (x < 0) ? 0 : x);
}


void write_pgm16 (char *fname, int16_t *rgb, int w, int h)
{
   int i;
   FILE *outfile;

   outfile = fopen (fname, "w");

   if (!outfile) {
      printf ("error opening '%s' for writing !!!\n", fname);
      exit (-1);
   }

   fprintf (outfile, "P5\n%d %d\n%d\n", w, h, 255);
   for (i=0; i<w*h; i++) {
      uint8_t c = CLAMP(rgb[i]);
      fwrite (&c, 1, 1, outfile);
   }
   fclose (outfile);
}


