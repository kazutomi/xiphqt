#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int _argc,char *_argv[]){
  FILE          *in_y;
  FILE          *in_u;
  FILE          *in_v;
  FILE          *out_yuv;
  int            y_w;
  int            y_h;
  int            c_w;
  int            c_h;
  char           ip;
  int            fps_n;
  int            fps_d;
  int            par_n;
  int            par_d;
  char          *chroma;
  unsigned char *buf;
  char           fname[8192];
  int            argi;
  if(_argc<2){
    fprintf(stderr,"Usage: qcif2yuv4mpeg <sequence_qcif> [<options]\n"
     "Converts <sequence>.y, <sequence>.u, and <sequence>.v into a single\n"
     "<sequence>.yuv file.\n"
     "\n"
     "The default input video format is 29.97 fps progressive QCIF.\n"
     "Options:\n"
     " -w<width>\n"
     " -h<height>\n"
     " -fn<frame rate numerator>\n"
     " -fd<frame rate denominator>\n"
     " -i<p|t|b>       Progressive or Top-field first or Bottom-field first.\n"
     " -an<pixel aspect numerator>\n"
     " -ad<pixel aspect denominator>\n"
     " -c<chroma>      Chroma subsampling format. One of:\n"
     "                   420jpeg\n"
     "                   420mpeg2\n"
     "                   420paldv\n"
     "                   411*\n"
     "                   422\n"
     "                   444\n"
     "                   444alpha*\n"
     "                   mono*\n"
     "                  *=Format not supported.\n");
    return -1;
  }
  if(strlen(_argv[1])>8187){
    fprintf(stderr,"File name too long.\n");
    return -1;
  }
  sprintf(fname,"%s.y",_argv[1]);
  in_y=fopen(fname,"rb");
  sprintf(fname,"%s.u",_argv[1]);
  in_u=fopen(fname,"rb");
  sprintf(fname,"%s.v",_argv[1]);
  in_v=fopen(fname,"rb");
  sprintf(fname,"%s.yuv",_argv[1]);
  out_yuv=fopen(fname,"wb");
  if(in_y==NULL||in_u==NULL||in_v==NULL||out_yuv==NULL){
    fprintf(stderr,"Error opening files.\n");
    return -1;
  }
  y_w=176;
  y_h=144;
  fps_n=30000;
  fps_d=1001;
  ip='p';
  par_n=128;
  par_d=117;
  chroma="420jpeg";
  for(argi=2;argi<_argc;argi++){
    if(_argv[argi][0]!='-'){
      fprintf(stderr,"Error parsing arguments: must start with '-'.\n");
      return -1;
    }
    switch(_argv[argi][1]){
      case 'w':y_w=atoi(_argv[argi]+2);break;
      case 'h':y_h=atoi(_argv[argi]+2);break;
      case 'i':ip=_argv[argi][2];break;
      case 'c':chroma=_argv[argi]+2;break;
      case 'f':{
        if(_argv[argi][2]=='n'){
          fps_n=atoi(_argv[argi]+3);
          break;
        }
        else if(_argv[argi][2]=='d'){
          fps_d=atoi(_argv[argi]+3);
          break;
        }
      }
      case 'a':{
        if(_argv[argi][2]=='n'){
          par_n=atoi(_argv[argi]+3);
          break;
        }
        else if(_argv[argi][2]=='d'){
          par_d=atoi(_argv[argi]+3);
          break;
        }
      }
      default:{
        fprintf(stderr,"Error parsing arguments: unknown switch '%c'.\n",
         _argv[argi][1]);
        return -1;
      }
    }
  }
  if(strncmp(chroma,"420",3)==0){
    c_w=y_w+1>>1;
    c_h=y_h+1>>1;
  }
  else if(strcmp(chroma,"422")==0){
    c_w=y_w+1>>1;
    c_h=y_h;
  }
  else if(strncmp(chroma,"444",3)==0){
    c_w=y_w;
    c_h=y_h;
  }
  else{
    fprintf(stderr,"Error parsing arguments: Unsupported chroma mode: %s.\n",
     chroma);
    return -1;
  }
  fprintf(out_yuv,"YUV4MPEG2 W%i H%i F%i:%i I%c A%i:%i",
   y_w,y_h,fps_n,fps_d,ip,par_n,par_d);
  if(strcmp(chroma,"420jpeg")!=0)fprintf(out_yuv," C%s",chroma);
  fprintf(out_yuv,"\n");
  buf=(unsigned char *)malloc((size_t)(y_w*y_h));
  for(;;){
    if(fread(buf,y_w,y_h,in_y)<y_h)break;
    fprintf(out_yuv,"FRAME\n");
    fwrite(buf,y_w,y_h,out_yuv);
    fread(buf,c_w,c_h,in_u);
    fwrite(buf,c_w,c_h,out_yuv);
    fread(buf,c_w,c_h,in_v);
    fwrite(buf,c_w,c_h,out_yuv);
  }
  return 0;
}
