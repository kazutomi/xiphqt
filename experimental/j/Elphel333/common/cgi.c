#include <fcntl.h>  /*open*/
#include <unistd.h> /* close, sleep */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <sys/types.h> /* for fstat */
#include <sys/stat.h> /* for fstat */

#include <sys/mman.h>    /* mmap */

#include <string.h>
#include <errno.h>
#include <math.h>



#include "cgi.h"

int hexdigit (char c) {
  int i;
  i=c-'0';
  if ((i>=0) && (i<10)) return i;
  i=c-'a'+10;
  if ((i>=10) && (i<16)) return i;
  i=c-'A'+10;
  if ((i>=10) && (i<16)) return i;
  return 0;  // could be -1??
}

int unescape (char * s, int l) {
  int i=0;
  int j=0;
  while ((i<l) && s[i]) {
    if ((s[i]=='%') && (i<(l-2)) && s[i+1]){ // behavior from Mozilla
      s[j++]=(hexdigit(s[i+1])<<4) | hexdigit(s[i+2]);
      i+=3;
    } else s[j++]=s[i++];
  }
  if (i<l) s[j]=0;
  return j;
}

int parseQuery(struct key_value * params, char * qry)
{
  char * cp;
  int l=0;
  cp=strtok(qry,"=");
  while ((cp) && (l<QRY_MAXPARAMS)) {
    params[l].key=cp;
    cp=strtok(NULL,"&");
    params[l++].value=cp;
    if (cp) cp=strtok(NULL,"=");
  }
 params[l].key=NULL;
 return l;
}

char * paramValue(struct key_value * params, char * skey)
{ // returns pointer to parameter value, NULL if not defined
  int i=0;
  if (skey)
   while ((i<QRY_MAXPARAMS) && (params[i].key)) {
    if (strcmp(skey,params[i].key)==0) return   params[i].value;

    i++;
   }
  return NULL;
}
