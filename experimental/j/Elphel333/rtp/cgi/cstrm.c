/******************************************************************************
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ******************************************************************************/
// CGI interface for Streamer on  Elphel JPEG camera 313 input device
// // Copyright (c) 2004 Andy Latin aka Landy, Dmitry Latin aka ipfreely
// // All rights reserved.
// //
// // http://andy.qnx.org.ru
// // mail to:
// // andy@qnx.org.ru
// // landy@mail333.com
// /////////////////////////////////////////////////////////////////////////////

#include <fcntl.h>  /*open*/
#include <unistd.h> /* close, sleep */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <sys/types.h> /* for fstat */
#include <sys/stat.h> /* for fstat */

#include <sys/mman.h>		/* mmap */

#include <string.h>
#include <errno.h>
#include <math.h>

#define QRY_MAXPARAMS 64
struct key_value {
	char *key;
	char *value;
};

struct key_value gparams[QRY_MAXPARAMS+1];

#define STOP_STREAM     0
#define START_STREAM    1
#define STOP_SCANNING   0
#define START_SCANNING  1

#define JPEG_ACCESS_MMAP 1
#define JPEG_ACCESS_DRV  2

typedef struct strm_common {
	char 	address[21];   	// address or hostname
	int	port;		// port no
	double	fps;		// soft FPS
	int     regim;		// mmap/driver access to JPEG data
	int	scanning;	// mode scanning on/off
	char 	name[21];	// SDES name field for subtitles
	char	telephone[21];  // phone subtitles
	char	toolname[21];
	char	note[61];
	char	location[41];
	char	email[21];
	char    encrypt_key[41];// encryption key
	int     encryption;	// mode encryption on/off
	int	status;		// start/top streamer
} strm_data_t;



/////////////////////// prototypes /////////////////////////////////////////
int parseQuery(struct key_value * params, char * qry);
char * paramValue(struct key_value * params, char * skey); // returns pointer to parameter value, NULL if not defined
int unescape (char * s, int l);
int hexdigit (char c);

///////////////////////////////////////////////////////////////////////////

int hexdigit (char c) {
  int i;
  i=c-'0';
  if ((i>=0) && (i<10)) return i;
  i=c-'a'+10;
  if ((i>=10) && (i<16)) return i;
  i=c-'A'+10;
  if ((i>=10) && (i<16)) return i;
  return 0;	// could be -1??
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
    if (strcmp(skey,params[i].key)==0) return 	params[i].value;

    i++;
   }
  return NULL;
}


////////////////////////////////////////////////////////////////////////
int main (int argc, char *argv[]) {
  int devfd,i,code;
  char * cp;
  char * vp;
/* these are from cgitest.cgi */
  const char *method;
  const char *query;
  const char *uri;
  char  copyQuery [4096];

  const char mmapFileName[]= "/var/tmp/mmap_strm";
  strm_data_t    *strm_data;

  method = getenv("REQUEST_METHOD");
  query  = getenv("QUERY_STRING");
  uri    = getenv("REQUEST_URI");
  i= (query)? strlen (query):0;
     fprintf(stderr,"querry length=%d\n", i);

  if (i>(sizeof(copyQuery)-1)) i= sizeof(copyQuery)-1;
  if (i>0) strncpy(copyQuery,query, i);
  copyQuery[i]=0;
  unescape(copyQuery,sizeof(copyQuery));

  if (method == NULL)
  {
    /* Not a CGI! */
    printf("This program should be run as a CGI from the web server!\n");
    exit(1);
  }

  parseQuery(gparams, copyQuery);
 // open mmap memory
  devfd = open(mmapFileName,O_RDWR);   //S_IRWXU);
  if(devfd == -1) {
	printf("Error create SHM data!\n");
  }
  lseek(devfd,sizeof(strm_data_t),SEEK_SET);
  strm_data = (strm_data_t *)mmap(0, sizeof(strm_data_t), PROT_READ|PROT_WRITE, MAP_SHARED, devfd, 0);
  if((int)strm_data == -1) {
        fprintf(stderr,"Error in mmap %s\r\n",mmapFileName);
        close(devfd);
	exit(1);
  }
  code = STOP_STREAM;
  if((vp=paramValue(gparams, "c")))  code = strtol(vp,&cp,10);
  if((strm_data->status == START_STREAM ) && code== START_STREAM) {
    // streamer already started! Restart it with new parameters
    strm_data->status = STOP_STREAM;
    // wait some time
    usleep(100000);
   }

    if ((vp=paramValue(gparams, "addr")))  strncpy(strm_data->address,vp,20);
    if ((vp=paramValue(gparams, "port")))  strm_data->port = strtol (vp,&cp,10);
    if ((vp=paramValue(gparams, "fps")))   strm_data->fps  = strtod(vp,&cp);
    if ((vp=paramValue(gparams, "scan")))  strm_data->scanning = strtol (vp,&cp,10);
    if ((vp=paramValue(gparams, "reg")))   strm_data->regim = strtol (vp,&cp,10);

    if ((vp=paramValue(gparams, "name")))  strncpy(strm_data->name,vp,20);
    if ((vp=paramValue(gparams, "loc")))   strncpy(strm_data->location,vp,40);
    if ((vp=paramValue(gparams, "note")))  strncpy(strm_data->note,vp,60);
    if ((vp=paramValue(gparams, "tool")))  strncpy(strm_data->toolname,vp,20);
    if ((vp=paramValue(gparams, "phone"))) strncpy(strm_data->telephone,vp,20);
    if ((vp=paramValue(gparams, "email"))) strncpy(strm_data->email,vp,20);
    if ((vp=paramValue(gparams, "key")))   strncpy(strm_data->encrypt_key,vp,40);
    if ((vp=paramValue(gparams, "encr")))  strm_data->encryption = strtol (vp,&cp,10);

    // OK - new parameters passed to mapped memory
    // start streamer

  strm_data->status = code;
  close(devfd);

 return 0;
}

