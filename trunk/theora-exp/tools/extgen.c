#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <string.h>



/*This utility generates the extrapolation matrices used to pad incomplete
   blocks when given frame sizes that are not a multiple of two.
  Although the technique used here is generalizable to non-rectangular shapes,
   the storage needed for the matrices becomes larger (exactly 13.75K, if we
   discount the superfluous mask, na, pi, and ci members of the
   oc_coeffcient_info structure and use a 256-byte mask-padding lookup table
   instead).*/



/*The matrix representation of the Theora's actual iDCT.
  This is computed using the 16-bit approximations of the sines and cosines
   that Theora uses, but using floating point values and assuming that no
   truncation occurs after division (i.e., pretending that the transform is
   linear like the real DCT).*/
static const double OC_IDCT_MATRIX[8][8]={
  {
    +0.7071075439453125,+0.9807891845703125,
    +0.9238739013671875,+0.8314666748046875,
    +0.7071075439453125,+0.555572509765625,
    +0.3826904296875,   +0.1950836181640625
  },
  {
    +0.7071075439453125,+0.8314685295335948,
    +0.3826904296875,   -0.1950868454296142,
    -0.7071075439453125,-0.9807858711574227,
    -0.9238739013671875,-0.5555783333256841
  },
  {
    +0.7071075439453125,+0.5555783333256841,
    -0.3826904296875,   -0.9807858711574227,
    -0.7071075439453125,+0.1950868454296142,
     0.9238739013671875,+0.8314685295335948
  },
  {
    +0.7071075439453125,+0.1950836181640625,
    -0.9238739013671875,-0.555572509765625,
    +0.7071075439453125,+0.8314666748046875,
    -0.3826904296875,   -0.9807891845703125
  },
  {
    +0.7071075439453125,-0.1950836181640625,
    -0.9238739013671875,+0.555572509765625,
    +0.7071075439453125,-0.8314666748046875,
    -0.3826904296875,   +0.9807891845703125
  },
  {
    +0.7071075439453125,-0.5555783333256841,
    -0.3826904296875,    0.9807858711574227,
    -0.7071075439453125,-0.1950868454296142,
    +0.9238739013671875,-0.8314685295335948
  },
  {
    +0.7071075439453125,-0.8314685295335948,
    +0.3826904296875,    +0.1950868454296142,
    -0.7071075439453125,+0.9807858711574227,
    -0.9238739013671875,+0.5555783333256841
  },
  {
    +0.7071075439453125,-0.9807891845703125,
    +0.9238739013671875,-0.8314666748046875,
    +0.7071075439453125,-0.555572509765625,
    +0.3826904296875,   -0.1950836181640625
  }
};

/*A table of the non-zero coefficients to keep for each possible shape.
  Our basis selection is chosen to optimize the coding gain.
  This gives us marginally better performance than other optimization criteria
   for the border extension case (and significantly better performance in the
   general case).*/
static const unsigned char OC_PAD_MASK_GAIN[256]={
  0x00,0x01,0x01,0x11,0x01,0x11,0x05,0x49,
  0x01,0x05,0x11,0x15,0x11,0x15,0xA1,0x55,
  0x01,0x05,0x11,0x15,0x11,0x29,0x51,0x55,
  0x03,0x85,0x19,0xA5,0x61,0x35,0xB1,0xAD,
  0x01,0x11,0x05,0x23,0x03,0x85,0x0D,0x2B,
  0x11,0x15,0x61,0x55,0x13,0x1B,0xA3,0x5B,
  0x11,0x83,0x83,0x55,0x13,0x93,0xC3,0xCB,
  0x61,0xC3,0x33,0x3B,0x99,0xCB,0xB3,0xDB,
  0x01,0x11,0x03,0x0B,0x05,0x0B,0x07,0x4B,
  0x11,0x07,0x07,0x1B,0x83,0x8B,0x87,0x57,
  0x11,0x23,0x07,0x33,0x61,0x2B,0x0F,0x57,
  0x19,0x87,0x1B,0x2F,0x33,0x8F,0x8F,0xB7,
  0x05,0x83,0x07,0x63,0x0D,0x87,0x87,0x6B,
  0x51,0x47,0x0F,0x67,0xC3,0xCB,0xE3,0xD7,
  0xA1,0xA3,0x87,0x73,0xA3,0x6B,0xE3,0xCF,
  0xB1,0x8F,0x8F,0xE7,0xB3,0xCF,0xE7,0xF7,
  0x01,0x03,0x11,0x43,0x11,0x31,0x83,0x93,
  0x05,0x07,0x23,0x27,0x83,0x87,0xA3,0x57,
  0x05,0x07,0x07,0xC3,0x15,0x0F,0x47,0x4F,
  0x85,0x87,0x87,0xA7,0xC3,0x8F,0x8F,0xAF,
  0x11,0x31,0x0B,0x1B,0x85,0x1B,0x87,0x2F,
  0x29,0x0F,0x2B,0x1F,0x93,0x1F,0x6B,0x5F,
  0x15,0x87,0x8B,0x57,0x1B,0x1F,0xCB,0x5F,
  0x35,0x8F,0x8F,0x3F,0xCB,0x9F,0xCF,0xDF,
  0x11,0x43,0x0B,0x33,0x23,0x1B,0x63,0x9B,
  0x15,0xC3,0x33,0x37,0x55,0x57,0x73,0x77,
  0x15,0x27,0x1B,0x37,0x55,0x1F,0x67,0x3F,
  0xA5,0xA7,0x2F,0xB7,0x3B,0x3F,0xE7,0xBF,
  0x49,0x93,0x4B,0x9B,0x2B,0x2F,0x6B,0x7B,
  0x55,0x4F,0x57,0x3F,0xCB,0x5F,0xCF,0x7F,
  0x55,0x57,0x57,0x77,0x5B,0x5F,0xD7,0x7F,
  0xAD,0xAF,0xB7,0xBF,0xDB,0xDF,0xF7,0xFF
};



/*Computes the Cholesky factorization L L^T of the matrix G^T G, where G is the
   currently selected bass functions restricted to the region of spatial
   support.
  The reciprocal of the diagonal element is stored instead of the diagonal
   itself, so that the division only needs to be done once.
  _l:   The L matrix to compute.
  _ac:  The set of basis functions used for each row.
  _ap:  The set of pixels that are not padding.
  _na:  The number of active pixels/coefficients.*/
static void oc_cholesky8(double _l[8][8],int _ac[8],int _ap[8],int _na){
  int aci;
  int acj;
  int ack;
  int api;
  int ci;
  int cj;
  int pi;
  for(aci=0;aci<_na;aci++){
    /*Step 1: Add the next row/column of G^T G.*/
    ci=_ac[aci];
    for(acj=0;acj<=aci;acj++){
      cj=_ac[acj];
      _l[aci][acj]=0;
      for(api=0;api<_na;api++){
        pi=_ap[api];
        _l[aci][acj]+=OC_IDCT_MATRIX[pi][cj]*OC_IDCT_MATRIX[pi][ci];
      }
    }
    /*Step 2: Convert the newly added row to the corresponding row of the
       Cholesky factorization.*/
    for(acj=0;acj<aci;acj++){
      for(ack=0;ack<acj;ack++)_l[aci][acj]-=_l[aci][ack]*_l[acj][ack];
      _l[aci][acj]*=_l[acj][acj];
    }
    for(ack=0;ack<aci;ack++)_l[aci][aci]-=_l[aci][ack]*_l[aci][ack];
    _l[aci][aci]=1/sqrt(_l[aci][aci]);
  }
}

/*Computes the padding extrapolation matrix for a single 1-D 8-point DCT.*/
int oc_calc_ext8(double _e[8][8],int _ap[8],int _ac[8],int _mask,int _pad){
  double b[8];
  double l[8][8];
  double w;
  int   api;
  int   zpi;
  int   aci;
  int   acj;
  int   ci;
  int   pi;
  int   na;
  int   nz;
  na=nz=0;
  for(pi=0;pi<8;pi++){
    if(_mask&1<<pi)_ap[na++]=pi;
    else _ap[8-++nz]=pi;
  }
  na=nz=0;
  for(ci=0;ci<8;ci++){
    if(_pad&1<<ci)_ac[na++]=ci;
    else _ac[8-++nz]=ci;
  }
  oc_cholesky8(l,_ac,_ap,na);
  for(api=0;api<na;api++){
    pi=_ap[api];
    for(aci=0;aci<na;aci++){
      b[aci]=OC_IDCT_MATRIX[pi][_ac[aci]];
      for(acj=0;acj<aci;acj++)b[aci]-=l[aci][acj]*b[acj];
      b[aci]*=l[aci][aci];
    }
    for(aci=na;aci-->0;){
      for(acj=aci+1;acj<na;acj++)b[aci]-=l[acj][aci]*b[acj];
      b[aci]*=l[aci][aci];
    }
    for(zpi=na;zpi<8;zpi++){
      pi=_ap[zpi];
      w=0;
      for(aci=0;aci<na;aci++)w+=OC_IDCT_MATRIX[pi][_ac[aci]]*b[aci];
      _e[zpi][api]=w;
    }
  }
  return na;
}

/*The precision at which the matrix coefficients are stored.*/
#define OC_EXT_SHIFT (19)

/*The number of shapes we need.*/
#define OC_NSHAPES   (35)

/*These are all the possible shapes that could be encountered in border
   extension.
  The first 14 correspond to the left and right borders (or top and bottom when
   considered vertically), and the next 21 arise from the combination of the
   two, to handle the case of a video less than 8 pixels wide (or tall).
  Order matters here, because we will search this array linearly for the entry
   to use during the actual transform.*/
static int masks[OC_NSHAPES]={
  0x7F,0xFE,0x3F,0xFC,0x1F,0xF8,0x0F,0xF0,0x07,0xE0,0x03,0xC0,0x01,0x80,
  0x7E,
  0x7C,0x3E,
  0x78,0x3C,0x1E,
  0x70,0x38,0x1C,0x0E,
  0x60,0x30,0x18,0x0C,0x06,
  0x40,0x20,0x10,0x08,0x04,0x02
};

int main(void){
  int   coeffs[8192];
  int   ncoeffs;
  int   ci;
  int   crstart[OC_NSHAPES*8];
  int   crend[OC_NSHAPES*8];
  int   ncr;
  int   cri;
  int   offsets[1024];
  int   noffsets;
  int   oi;
  int   orstart[OC_NSHAPES*8];
  int   orend[OC_NSHAPES*8];
  int   nor;
  int   ori;
  int   ap[OC_NSHAPES][8];
  int   ac[OC_NSHAPES][8];
  int   na[OC_NSHAPES];
  int   zpi;
  int   mi;
  ncoeffs=ncr=0;
  /*We store all the coefficients from the extension matrices in one large
     table.
    This lets us exploit the repetitiveness and overlap of many of the rows to
     reduce the size of the tables by about 49%.
    Our overlap search strategy is pretty simple, so we could probably do even
     better, but further improvement would be small.*/
  for(mi=0;mi<OC_NSHAPES;mi++){
    double e[8][8];
    int    api;
    na[mi]=oc_calc_ext8(e,ap[mi],ac[mi],masks[mi],OC_PAD_MASK_GAIN[masks[mi]]);
    for(zpi=na[mi];zpi<8;zpi++){
      int cr[8];
      int best_ci;
      int best_cre;
      int cj;
      for(api=0;api<na[mi];api++){
        cr[api]=(int)(e[zpi][api]*(1<<OC_EXT_SHIFT)+(e[zpi][api]<0?-0.5F:0.5F));
      }
      best_ci=ncoeffs;
      best_cre=0;
      for(ci=0;ci<ncoeffs;ci++){
        for(cj=ci;cj<ncoeffs&&cj-ci<na[mi]&&coeffs[cj]==cr[cj-ci];cj++);
        if(cj-ci>best_cre){
          if(cj-ci<na[mi]){
            /*If we need to extend the range, check to make sure we aren't
               interrupting any other range.*/
            for(cri=0;cri<ncr&&(crstart[cri]>=cj||crend[cri]<=cj);cri++);
            if(cri>=ncr){
              /*No conflicting ranges were found, so we can insert the
                 necessary coefficients.*/
              best_ci=ci;
              best_cre=cj-ci;
            }
          }
          else{
            /*Otherwise, we have a complete match, so we can stop searching.*/
            best_ci=ci;
            best_cre=cj-ci;
            break;
          }
        }
      }
      if(best_cre<na[mi]){
        /*We don't have a complete match, so we need to insert the remaining
           coefficients.*/
        memmove(coeffs+best_ci+na[mi],coeffs+best_ci+best_cre,
         sizeof(coeffs[0])*(ncoeffs-best_ci-best_cre));
        /*And now we need to update all the ranges that start after the
           insertion point.*/
        for(cri=0;cri<ncr;cri++){
          if(crstart[cri]>=best_ci+best_cre){
            crstart[cri]+=na[mi]-best_cre;
            crend[cri]+=na[mi]-best_cre;
          }
        }
      }
      /*Actually add the coefficients.*/
      for(cj=best_cre;cj<na[mi];cj++)coeffs[best_ci+cj]=cr[cj];
      ncoeffs+=na[mi]-best_cre;
      /*Store the endpoints of the range.*/
      crstart[ncr]=best_ci;
      crend[ncr]=best_ci+na[mi];
      ncr++;
    }

  }
  printf("/*The precision at which the matrix coefficients are stored.*/\n");
  printf("#define OC_EXT_SHIFT (%i)\n\n",OC_EXT_SHIFT);
  printf("/*The number of shapes we need.*/\n");
  printf("#define OC_NSHAPES   (%i)\n\n",OC_NSHAPES);
  printf("static const ogg_int32_t OC_EXT_COEFFS[%i]={",ncoeffs);
  for(ci=0;ci<ncoeffs;ci++){
    if((ci&7)==0)printf("\n  ");
    printf("%c0x%05X",coeffs[ci]<0?'-':' ',abs(coeffs[ci]));
    if(ci+1<ncoeffs)printf(",");
  }
  printf("\n};\n\n");
  /*Now we repeat the same overlap strategy on the row pointers.
    This only gets us around 45% space reduction, but that's still worth it.*/
  noffsets=nor=0;
  for(cri=mi=0;mi<OC_NSHAPES;mi++){
    int best_oi;
    int best_ore;
    int oj;
    int nz;
    best_oi=noffsets;
    best_ore=0;
    nz=8-na[mi];
    for(oi=0;oi<noffsets;oi++){
      for(oj=oi;oj<noffsets&&oj-oi<nz&&offsets[oj]==crstart[cri+oj-oi];
       oj++);
      if(oj-oi>best_ore){
        if(oj-oi<nz){
          /*If we need to extend the range, check to make sure we aren't
             interrupting any other range.*/
          for(ori=0;ori<nor&&(orstart[ori]>=oj||orend[ori]<=oj);ori++);
          if(ori>=nor){
            /*No conflicting ranges were found, so we can insert the
               necessary coefficients.*/
            best_oi=oi;
            best_ore=oj-oi;
          }
        }
        else{
          /*Otherwise, we have a complete match, so we can stop searching.*/
          best_oi=oi;
          best_ore=oj-oi;
          break;
        }
      }
    }
    if(best_ore<nz){
      /*We don't have a complete match, so we need to insert the remaining
         offsets.*/
      memmove(offsets+best_oi+nz,offsets+best_oi+best_ore,
       sizeof(offsets[0])*(noffsets-best_oi-best_ore));
      /*And now we need to update all the ranges that start after the
         insertion point.*/
      for(ori=0;ori<nor;ori++){
        if(orstart[ori]>=best_oi+best_ore){
          orstart[ori]+=nz-best_ore;
          orend[ori]+=nz-best_ore;
        }
      }
    }
    /*Actually add the offsets.*/
    for(oj=best_ore;oj<nz;oj++)offsets[best_oi+oj]=crstart[cri+oj];
    noffsets+=nz-best_ore;
    /*Store the endpoints of the range.*/
    orstart[nor]=best_oi;
    orend[nor]=best_oi+nz;
    nor++;
    cri+=nz;
  }
  printf("static const ogg_int32_t *const OC_EXT_ROWS[%i]={",noffsets);
  for(oi=0;oi<noffsets;oi++){
    if((oi&3)==0)printf("\n  ");
    printf("OC_EXT_COEFFS+%4i",offsets[oi]);
    if(oi+1<noffsets)printf(",");
  }
  printf("\n};\n\n");
  printf("static const oc_extension_info OC_EXTENSION_INFO[OC_NSHAPES]={\n");
  for(mi=0;mi<OC_NSHAPES;mi++){
    int i;
    printf("  {");
    printf("0x%02X,%i,OC_EXT_ROWS+%3i,",masks[mi],na[mi],orstart[mi]);
    printf("{");
    for(i=0;i<8;i++){
      printf("%i",ap[mi][i]);
      if(i<7)printf(",");
    }
    printf("},{");
    for(i=0;i<8;i++){
      printf("%i",ac[mi][i]);
      if(i<7)printf(",");
    }
    printf("}}");
    if(mi+1<OC_NSHAPES)printf(",");
    printf("\n");
  }
  printf("};\n");
  return 0;
}
