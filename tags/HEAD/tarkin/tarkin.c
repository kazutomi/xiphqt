// Tarkin 'fixed' demo code
//
// Sample fixed-length encoding top layer. Limited to 1 block of 32
// frames. A lot of work will need to be done here in order to make
// a nice, tidy interface that'll convert .avi's, .mov's or whatever
// else... for the time being, it only converts a series of 32 .pgm
// files named "0.pgm" "1.pgm" .. "31.pgm".
//
// Usage: tarkin [percentage] (where percentage is % of DWT vectors to retain)
//
// $Id: tarkin.c,v 1.1 2001/02/13 01:06:24 giles Exp $
//
// $Log: tarkin.c,v $
// Revision 1.1  2001/02/13 01:06:24  giles
// Initial revision
//

#include "tarkin.h"

// Global Definitions
#define FRAMES 32

// Global Variables
tarkindata td;

// Function declarations
int sortfunc(const unsigned int *a, const unsigned int *b);
void dieusage();

int main(int argc, char **argv)
{
  uint *sortarr;
  int a, d, x, y, z;
  float percent;
  unsigned char ln[256], *data;
  FILE *fi;
  oggpack_buffer o;
  ulong bytes, dims[3];

// Future home of options processing here
  if(argc<2) dieusage();
  if(!(percent = (float)atof(argv[1]))) dieusage();

// For this code, we're using .pgms. Take the image dimensions from the first 
// frame and set up the tarkindata struct, then grab image data to populate the
// workspace.
  if(!(fi = fopen("0.pgm", "r"))) { perror("Can't open file 0.pgm"); exit(1); }
  for(a=0;a<3;a++) {
    fgets(ln, 255, fi);
    if(*ln == '#') a--;
    else {
      if(!a && strncmp("P5", ln, 2)) { fprintf(stderr, "Error: Need PGM file for input\n"); exit(0); }
      if(a==1) {
        ln[20] = 0;
        sscanf(ln, "%ld %ld", &(td.x_dim), &(td.y_dim));
      }
    }
  }
  td.z_dim = FRAMES;
  for(a=0,d=1;d<td.x_dim;a++,d<<=1); td.x_bits = a; td.x_workspace = d;
  for(a=0,d=1;d<td.y_dim;a++,d<<=1); td.y_bits = a; td.y_workspace = d;
  for(a=0,d=1;d<td.z_dim;a++,d<<=1); td.z_bits = a; td.z_workspace = d;
  td.sz = td.x_dim*td.y_dim*td.z_dim;
  td.sz_workspace = td.x_workspace*td.y_workspace*td.z_workspace;
  td.vectors = (float *)calloc(td.sz_workspace, sizeof(float));
  data = malloc(td.sz_workspace);
  for(y=0;y<td.y_dim;y++) fread(data+y*td.x_workspace, td.x_dim, 1, fi);
  fclose(fi);
  for(z=1;z<td.z_dim;z++) {
    sprintf(ln, "%d.pgm", z);
    if(!(fi = fopen(ln, "r"))) { fprintf(stderr, "Error opening "); perror(ln); exit(1); }
    for(a=0;a<3;a++) { fgets(ln, 256, fi); if(*ln == '#') a--; }
    for(y=0;y<td.y_dim;y++) fread(data+z*td.x_workspace*td.y_workspace+y*td.x_workspace, td.x_dim, 1, fi);
    fclose(fi);
  }
  for(a=0;a<td.sz_workspace;a++) td.vectors[a] = data[a];
  free(data);

  // Execute DWT transform
  dims[0] = td.x_workspace; dims[1] = td.y_workspace; dims[2] = td.z_workspace;
  dwt(td.vectors, dims, 3, 1);

  // Filter DWT vectors
  sortarr = (uint *)calloc(td.sz_workspace+1, sizeof(uint));
  for(a=0;a<td.sz_workspace;a++) sortarr[a]=a;
  qsort(sortarr, td.sz_workspace, sizeof(uint), &sortfunc);
  td.vectorcount = (td.sz*percent/100);
  for(x=td.vectorcount;x<td.sz_workspace;x++) td.vectors[sortarr[x]]=0;
  
  // Pack vectors into bitstream
  packblock(&td, &o);

  // Write bitstream
  fi = fopen("output.tark", "w");
  fprintf(fi, "%d %d %d\n", td.x_dim, td.y_dim, td.z_dim);
  data  = _oggpack_buffer(&o);
  bytes = _oggpack_bytes(&o);
  fwrite(data, bytes, 1, fi);
  fclose(fi);
}

int sortfunc(const unsigned int *a, const unsigned int *b)
{
  uint aa, bb;
  float f;

  aa = (uint)*a; bb = (uint)*b;
  f = (fabs(td.vectors[bb])-fabs(td.vectors[aa]));
  if(f==0) return 0;
  return(f>0 ? 1 : -1);
}

void dieusage()
{
  fprintf(stderr, "Usage:\ntarkin [percent]\n\n[percent] - Percent of DWT vectors to retain (can be fractional)\n");
  exit(1);
}

