#include <stdio.h>

int main(int _argc,char **_argv){
  FILE          *f1;
  FILE          *f2;
  unsigned char *buf1;
  unsigned char *buf2;
  int            w;
  int            h;
  int            frame;
  if(_argc!=5){
    fprintf(stderr,"Usage %s <file1> <file2> <width> <height>\n",_argv[0]);
    return 2;
  }
  w=atoi(_argv[3]);
  h=atoi(_argv[4]);
  f1=fopen(_argv[1],"rb");
  f2=fopen(_argv[2],"rb");
  buf1=(unsigned char *)malloc(sizeof(char)*(w*h*3/2));
  buf2=(unsigned char *)malloc(sizeof(char)*(w*h*3/2));
  for(frame=0;;frame++){
    unsigned char *p1;
    unsigned char *p2;
    int pli;
    int bx;
    int by;
    int bi;
    if(fread(buf1,w*h*3/2,1,f1)<1)break;
    if(fread(buf2,w*h*3/2,1,f2)<1)break;
    p1=buf1;
    p2=buf2;
    for(bi=pli=0;pli<3;pli++){
      for(by=0;by<(h>>(pli>0));by+=8){
        for(bx=0;bx<(w>>(pli>0));bx+=8){
          int x;
          int y;
          for(y=0;y<8;y++){
            for(x=0;x<8;x++){
              if(p1[((h>>(pli>0))-1-by-y)*(w>>(pli>0))+bx+x]!=
               p2[((h>>(pli>0))-1-by-y)*(w>>(pli>0))+bx+x]){
                break;
              }
            }
            if(x<8)break;
          }
          if(y<8){
            printf("Files differ at frame %i, block %i (%i,%i,%i)\n",
             frame,bi,pli,bx,by);
            for(y=0;y<8;y++){
              for(x=0;x<8;x++){
                printf("%02X%c",p1[((h>>(pli>0))-1-by-y)*(w>>(pli>0))+bx+x],
                 x<7?' ':'\n');
              }
            }
            printf("\n");
            for(y=0;y<8;y++){
              for(x=0;x<8;x++){
                printf("%02X%c",p2[((h>>(pli>0))-1-by-y)*(w>>(pli>0))+bx+x],
                 x<7?' ':'\n');
              }
            }
            printf("\n");
          }
          bi++;
        }
      }
      p1+=(w>>(pli>0))*(h>>(pli>0));
      p2+=(w>>(pli>0))*(h>>(pli>0));
    }
  }
  printf("Files are identical up to frame %i.\n",frame);
  return 0;
}
