// $Id: bitpack.c,v 1.1 2001/02/13 01:06:24 giles Exp $
//
// $Log: bitpack.c,v $
// Revision 1.1  2001/02/13 01:06:24  giles
// Initial revision
//

#include "tarkin.h"

void pack3d(tarkindata *td, oggpack_buffer *o)
{
  int x, y, z, wx, wy, fromx, fromy, tox, toy, cidx, cidold;
  uint xbit, ybit, vx, vy;
  ulong zbase, linaddr, vectorcount;

  cidx = 0;
  cidold = 0;

  for(z=0;z<td->z_workspace;z++) {
    zbase = z*td->x_workspace*td->y_workspace;
    for(ybit=0;ybit<td->y_bits;ybit++) {
      for(xbit=0;xbit<td->x_bits;xbit++) {
        fromx = (1 << xbit) - (xbit==0); fromy = (1 << ybit) - (ybit==0);
        tox   = 1 << (xbit+1);           toy   = 1 << (ybit+1);
        wx    = xbit + (xbit==0);        wy    = ybit + (ybit==0);

        // Scan for vectors in array between (fromx..tox, fromy..toy)
        for(vectorcount=0,y=fromy;y<toy;y++) for(x=fromx;x<tox;x++) {
          linaddr = zbase+y*td->x_workspace+x;
          if(td->vectors[linaddr]!=0) {
            // save global vector address in array
            td->dwtv[cidx].addr  = linaddr;
            td->dwtv[cidx++].mag = td->vectors[linaddr];
            vectorcount++;
          }
        }

        // Pack count of vectors found into (wx+wy+1) bits
// printf("%ld(%d)\n", vectorcount, wx+wy+1);
        _oggpack_write(o, vectorcount, wx+wy+1);

        for(;cidold<cidx;cidold++) {
          // Convert vector address into vx, vy
          linaddr = td->dwtv[cidold].addr - zbase;
          vx = linaddr % td->x_workspace; vy = linaddr / td->x_workspace;

          // Make vx/vy refer from domain origin
          vx -= fromx; vy -= fromy;

          // pack vx and vy into 1<<wx and 1<<wy bits respectively
          _oggpack_write(o, vx, wx);
          _oggpack_write(o, vy, wy);
// printf("%lu %d(%d) %d(%d)\n", cidold, vx, wx, vy, wy);
// printf("%lu\n", td->dwtv[cidold].addr);
        }
      }
    }
  }
}

void unpack3d(tarkindata *td, oggpack_buffer *o)
{
  int x, y, z, wx, wy, fromx, fromy;
  uint  xbit, ybit, vx, vy;
  ulong zbase, linaddr, cidx, vectorcount;

  cidx = 0;

  for(z=0;z<td->z_workspace;z++) {
// printf("Frame %d\n", z);
    zbase = z*td->x_workspace*td->y_workspace;
    for(ybit=0;ybit<td->y_bits;ybit++) {
      for(xbit=0;xbit<td->x_bits;xbit++) {
        fromx = (1 << xbit) - (xbit==0); fromy = (1 << ybit) - (ybit==0);
        wx    = xbit + (xbit==0);        wy    = ybit + (ybit==0);

	// Grab the vector count for this domain
        vectorcount = _oggpack_read(o, wx+wy+1);
// printf("%ld(%d)\n", vectorcount, wx+wy+1);

        // Grab the vectors (if any) and populate the dwtvec array with coords
        for(;vectorcount>0;vectorcount--) {
          vx = _oggpack_read(o, wx);
          vy = _oggpack_read(o, wy);
// printf("%lu %d(%d) %d(%d)\n", cidx, vx, wx, vy, wy);
// fflush(stdout);
          vx += fromx; vy += fromy;
          linaddr = vx+vy*td->x_workspace+zbase;
          td->dwtv[cidx++].addr = linaddr;
// printf("%lu\n", linaddr);
// fflush(stdout);
        }
      }
    }
  }
}

// packblock() and unpackblock() assume a packing schema of successive layers of 2d frames
// An attempt should be made at full multidimensional packing at some point, to see if
// some additional crunching of vector coordinates can take place.
//
// packblock assumes an empty oggpack_buffer, which gets populated with vector bits

void packblock(tarkindata *td, oggpack_buffer *oggb)
{
  int a, d, n;
  short int val;
  ulong l;
  float f;

  if(!td->dwtv) td->dwtv = (dwt_vector *)malloc(td->vectorcount*sizeof(dwt_vector));
  if(!td->dwtv) {
    fprintf(stderr, "Out of memory in packblock()\nFatal Error, program halted.\n");
    exit(-1);
  }
  bzero(td->dwtv, td->vectorcount*sizeof(dwt_vector));

  _oggpack_writeinit(oggb);
  _oggpack_write(oggb, td->vectorcount, sizeof(ulong)*8);

  pack3d(td, oggb);

// Replace this bit with the codebook stuff
// printf("short int: %d\n", sizeof(short int));
  for(a=0;a<td->vectorcount;a++) {
    val = (short int)((td->dwtv[a].mag)/4);
    f = td->dwtv[a].mag;
    memcpy(&l, &f, 4);
//    printf("%lu %d\n", td->dwtv[a].addr, val);
    _oggpack_write(oggb, l, 32);
  }
}

// unpackblock assumes the oggpack_buffer has already been init'd with data waiting
void unpackblock(tarkindata *td, oggpack_buffer *oggb)
{
  int a;
  short int val;
  ulong l;
  float f;

  td->vectorcount = _oggpack_read(oggb, sizeof(ulong)*8);
// printf("Vectorcount: %ld\n", td->vectorcount);

  if(!td->dwtv) td->dwtv = (dwt_vector *)malloc(td->vectorcount*sizeof(dwt_vector));
  if(!td->dwtv) {
    fprintf(stderr, "Out of memory in unpackblock()\nFatal Error, program halted.\n");
    exit(-1);
  }
  bzero(td->dwtv, td->vectorcount*sizeof(dwt_vector));

  unpack3d(td, oggb);
  td->vectors = (float *)calloc(td->sz_workspace, sizeof(float));

  // Replace this bit with the codebook stuff
  for(a=0;a<td->vectorcount;a++) {
    l = _oggpack_read(oggb, 32);
    memcpy(&f, &l, 4);
    td->vectors[td->dwtv[a].addr] = f;
//    printf("%lu %d\n", td->dwtv[a].addr, val);
  }
}
