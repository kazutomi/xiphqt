#include <stdio.h>
#include <string.h>
#include <ogg/ogg.h>

#define OC_NMODES (8)
#define OC_MINI(_a,_b) ((_a)>(_b)?(_b):(_a))

ogg_uint16_t OC_RES_BITRATES[64][3][OC_NMODES][16];

static ogg_int64_t OC_RES_BITRATE_ACCUM[64][3][OC_NMODES][16];
static int         OC_RES_BITRATE_SAMPLES[64][3][OC_NMODES][16];


int main(void){
  static const char *pl_names[3]={"Y'","Cb","Cr"};
  static const char *mode_names[OC_NMODES]={
    "OC_MODE_INTRA",
    "OC_MODE_INTER_NOMV",
    "OC_MODE_INTER_MV",
    "OC_MDOE_INTER_MV_LAST",
    "OC_MODE_INTER_MV_LAST2",
    "OC_MODE_INTER_MV_FOUR",
    "OC_MODE_GOLDEN_NOMV",
    "OC_MODE_GOLDEN_MV"
  };
  FILE *in;
  int   qi;
  int   pli;
  int   modei;
  int   erri;
  in=fopen("modedec.stats","rb");
  if(in==NULL)return;
  fread(OC_RES_BITRATE_ACCUM,sizeof(OC_RES_BITRATE_ACCUM),1,in);
  fread(OC_RES_BITRATE_SAMPLES,sizeof(OC_RES_BITRATE_SAMPLES),1,in);
  /*Update the current bitrate statistics in use.*/
  printf("ogg_uint16_t OC_RES_BITRATES[64][3][OC_NMODES][16]={\n");
  for(qi=0;qi<64;qi++){
    printf("  {\n");
    for(pli=0;pli<3;pli++){
      printf("    {\n");
      for(modei=0;modei<OC_NMODES;modei++){
        printf("      /*%s  qi=%i  %s*/\n",pl_names[pli],qi,
         mode_names[modei]);
        printf("      {\n");
        printf("        ");
        for(erri=0;erri<16;erri++){
          int n;
          if(erri==8)printf("\n        ");
          n=OC_RES_BITRATE_SAMPLES[qi][pli][modei][erri];
          if(!n)printf("%5i",0);
          else{
            OC_RES_BITRATES[qi][pli][modei][erri]=(ogg_uint16_t)OC_MINI(65535,
             ((OC_RES_BITRATE_ACCUM[qi][pli][modei][erri]<<1)+n)/(n<<1));
            printf("%5i",OC_RES_BITRATES[qi][pli][modei][erri]);
          }
          if(erri<15)printf(",");
        }
        printf("\n");
        if(modei<OC_NMODES-1)printf("      },\n");
        else printf("      }\n");
      }
      if(pli<2)printf("    },\n");
      else printf("    }\n");
    }
    if(qi<63)printf("  },\n");
    else printf("  }\n");
  }
  printf("};\n");
}
