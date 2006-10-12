#define _REENTRANT 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ogg/ogg2.h>
#include "codec.h"

void usage(FILE *out){
  fprintf(out,
	  "\nVorbose 20030722-1\n"
	  "Vorbis I header/stream information dump tool\n\n"

	  "USAGE:\n"
	  "  vorbose [options] < vorbis-stream\n\n"

	  "OPTIONS:\n"
	  "  -c --codebook-info       : parse and dump detailed stream codebooks\n"
	  "  -g --page-info           : parse and dump info about each stream page\n"
	  "  -h --help                : print this usage message to stdout and exit\n"
	  "                             with status zero\n"
	  "  -H --header-info         : parse and dump high-level header contents\n"
	  "                             and decoder setup\n"
	  "  -p --packet-info         : output basic information for each Vorbis\n"
	  "                             stream packet\n"
	  "  -s --stream-info         : output basic information about Ogg stream\n"
	  "                             used as Vorbis container\n"
	  "  -t --truncated-packets   : check stream for truncated audio packets\n"
	  "                             (truncated packets are perfectly legal)\n"
	  "  -v --verbose             : turn on all reports\n"
	  "  -w --warnings            : report anything fishy in the stream\n"
	  "\n"
	  );
}

const char *optstring = "cgHhpstvw";

struct option options [] = {
  {"codebook-info",no_argument,NULL,'c'},
  {"page-info",no_argument,NULL,'g'},
  {"help",no_argument,NULL,'h'},
  {"header-info",no_argument,NULL,'H'},
  {"packet-info",no_argument,NULL,'p'},
  {"stream-info",no_argument,NULL,'s'},
  {"truncated-packets",no_argument,NULL,'t'},
  {"verbose",no_argument,NULL,'v'},
  {"warnings",no_argument,NULL,'w'},
  {NULL,0,NULL,0}
};

static ogg2_sync_state   *oy;
static ogg2_stream_state *os;
static ogg2_page          og;
static ogg2_packet        op;
static vorbis_info       vi;

int codebook_p=0;
int pageinfo_p=0;
int headerinfo_p=0;
int packetinfo_p=0;
int streaminfo_p=0;
int truncpacket_p=0;
int warn_p=0;
int syncp=1;

int get_data(){
  unsigned char *buf;
  int ret;
  buf = ogg2_sync_bufferin(oy,256);
  
  if(!buf){
    fprintf(stderr,"ERROR internal: Failed to allocate managed buffer; ogg2_sync_buffer() failed.\n");
    exit(1);
  }

  ret=fread(buf,1,256,stdin);
  if(ret>0)ogg2_sync_wrote(oy,ret);
  return ret;
}

void dump_page(ogg2_page *og){
  ogg2pack_buffer *opb=alloca(ogg2pack_buffersize());
  unsigned long ret,ret2,flag;
  unsigned int i,count=0,postp;
  unsigned long lacing[255];

  ogg2pack_readinit(opb,og->header);
  /* capture pattern */
  ogg2pack_read(opb,32,&ret);
  ogg2pack_read(opb,8,&ret2);
  printf("INFO   page: Capture pattern %c%c%c%c, ",
	 (char)(ret),(char)(ret>>8),(char)(ret>>16),(char)(ret>>24));
  /* version */
  printf("format version %lu\n",ret2);
  /* flags */
  ogg2pack_read(opb,8,&flag);
  printf("             Flags: %s%s%s",
	 (flag&1 ? 
	  "packet continued from previous page"
	  "\n                    ":""),
	 (flag&2 ? 
	  "first page of logical stream"
	  "\n                    ":""),
	 (flag&4 ? 
	  "last page of logical stream"
	  "\n                    ":""));
  if(!(flag&7))printf("none\n");
  
  /* granpos */
  ogg2pack_read(opb,32,&ret);
  ogg2pack_read(opb,32,&ret2);
  printf("\n             Granule position: 0x%08lx%08lx\n",ret2,ret);
  /* serial number */
  ogg2pack_read(opb,32,&ret);
  printf("             Stream serialno : 0x%08lx\n",ret);
  /* sequence number */
  ogg2pack_read(opb,32,&ret);
  printf("             Sequence number : %ld\n",ret);
  /* checksum */
  ogg2pack_read(opb,32,&ret);
  printf("             Checksum        : 0x%08lx\n",ret);
  /* segments (packets) */
  ogg2pack_read(opb,8,&ret);
  printf("             Total segments  : %ld\n",ret);
  /* segment list and packet count */
  for(i=0;i<ret;i++){
    ogg2pack_read(opb,8,lacing+i);
    postp=1;
    if(lacing[i]<255){
      count++;
      postp=0;
    }
  }
  printf("             Total packets   : ");
  if(flag&1){
    if(count==0){
      printf("single incomplete spanning packet");
    }else{
      if(postp){
	printf("%d completed (1 cont), 1 incomplete",count);
      }else{
	printf("%d completed (1 cont)",count);
      }
    }
  }else{
    if(postp){
      printf("%d completed, 1 incomplete",count);
    }else{
      printf("%d completed",count);
    }
  }
  printf("\n                              (");
  for(i=0;i<ret;i++){
    if(i%8==0 && i!=0)
      printf("\n                               ");
    printf("%3lu",lacing[i]);
    if(i+1<ret && (i+1)%8)
      printf(", ");
  }
  printf(")\n\n");
}

int main(int argc,char *argv[]){
  int c,long_option_index;
  int eof=0;
  int vorbiscount=0;

  /* get options */
  while((c=getopt_long(argc,argv,optstring,options,&long_option_index))!=EOF){
    switch(c){
    case 'c':
      codebook_p=1;
      break;
    case 'g':
      pageinfo_p=1;
      break;
    case 'h':
      usage(stdout);
      exit(0);
    case 'H':
      headerinfo_p=1;
      break;
    case 'p':
      packetinfo_p=1;
      break;
    case 's':
      streaminfo_p=1;
      break;
    case 't':
      truncpacket_p=1;
      break;
    case 'v':
      codebook_p=1;
      pageinfo_p=1;
      headerinfo_p=1;
      packetinfo_p=1;
      streaminfo_p=1;
      truncpacket_p=1;
      warn_p=1;
      break;
    case 'w':
      warn_p=1;
      break;
    default:
      usage(stderr);
      exit(1);
    }
  }

  /* set up sync */
  
  oy=ogg2_sync_create(); 
  os=ogg2_stream_create(0);


  while(!eof){
    long ret;
    long garbagecounter=0;
    long pagecounter=0;
    long packetcounter=0;
    int initialphase=0;

    memset(&vi,0,sizeof(vi));

    /* packet parsing loop */
    while(1){
      
      /* is there a packet available? */
      if(ogg2_stream_packetout(os,&op)>0){
	/* yes, process it */

	if(packetcounter<3){
	  /* header packet */
	  ret=vorbis_info_headerin(&vi,&op);
	  if(ret){
	    switch(packetcounter){
	    case 0: /* initial header packet */
	      if((streaminfo_p || warn_p || headerinfo_p) && syncp)
		printf("WARN stream: page did not contain a valid Vorbis I "
		       "identification\n"
		       "             header. Stream is not decodable as "
		       "Vorbis I.\n\n");
	      break;
	    case 1:
	      if((streaminfo_p || warn_p || headerinfo_p) && syncp)
		printf("WARN stream: next packet is not a valid Vorbis I "
		       "comment header as expected.\n"
		       "             Stream is not decodable as "
		       "Vorbis I.\n\n");
	      break;
	    case 2:
	      if((streaminfo_p || warn_p || headerinfo_p) && syncp)
		printf("WARN stream: next packet is not a valid Vorbis I "
		       "setup header as expected.\n"
		       "             Stream is not decodable as "
		       "Vorbis I.\n\n");
	      
	      break;
	    }
	    syncp=0;
	  }else{
	    syncp=1;
	    packetcounter++;
	    if(packetcounter==3){
	      if(streaminfo_p || headerinfo_p)
		printf("info stream: Vorbis I header triad parsed successfully.\n\n");
	      vorbiscount++;
	    }
	  }
	}else{
	  /* audio packet */

	  vorbis_decode(&vi,&op);
	  packetcounter++;
	}
	continue;
      }

      /* is there a page available? */
	  
      ret=ogg2_sync_pageseek(oy,&og);
      if(ret<0){
	garbagecounter-=ret;
      }
      if(ret>0){
	/* Garbage between pages? */
	if(garbagecounter){
	  if(streaminfo_p || warn_p || pageinfo_p)
	    fprintf(stdout,"WARN stream: %ld bytes of garbage before page %ld\n\n",
		    garbagecounter,pagecounter);
	  garbagecounter=0;
	}

	if(initialphase && !ogg2_page_bos(&og)){
	  /* initial header pages phase has ended */
	  if(streaminfo_p || headerinfo_p){
	    printf("info stream: All identification header pages parsed.\n"
		   "             %d logical stream%s muxed in this link.\n\n",
		   initialphase,(initialphase==1?"":"s"));
	    if(initialphase>1 && (warn_p || streaminfo_p))
	      printf("WARN stream: A 'Vorbis I audio stream' must contain uninterleaved\n"
		     "             Vorbis I logical streams only.  This is a legal\n"
		     "             multimedia Ogg stream, but not a Vorbis I audio\n"
		     "             stream.\n\n");
	  }
	  initialphase=0;
	}

	if(pageinfo_p)dump_page(&og);


	/* is this a stream transition? */
	if(ogg2_page_bos(&og) || pagecounter==0){
	  if(initialphase){
	    /* we're in a muxed stream, which is illegal for Vorbis I
               audio-only, but perfectly legal for Ogg. */
	    if(!syncp){
	      /* we've not yet seen the Vorbis header go past; keep trying new streams */
	      ogg2_stream_reset_serialno(os,ogg2_page_serialno(&og));
	    }
	  }else{
	    /* first new packet, signals new stream link.  Dump the current vorbis stream, if any */
	    ogg2_stream_reset_serialno(os,ogg2_page_serialno(&og));
	    memset(&vi,0,sizeof(vi));
	    packetcounter=0;
	    vorbis_info_clear(&vi);
	  }
	  initialphase++;

	  /* got an initial page.  Is it beginning of stream? */
	  if(!ogg2_page_bos(&og) && pagecounter==0 && streaminfo_p)
	    if(warn_p || streaminfo_p)
	      fprintf(stdout,"WARN stream: first page (0) is not marked beginning of stream.\n\n");
	}	
	
	ogg2_stream_pagein(os,&og);
	pagecounter++;
	continue;
      }
    
      if(get_data()<=0){
	eof=1;
	break;
      }
    }
  }

  if(streaminfo_p)
    fprintf(stdout, "\nHit physical end of stream at %ld bytes.\n",
	    ftell(stdin));

  if(vorbiscount==0)
    fprintf(stdout,"No logical Vorbis streams found in data.\n");
  else
    fprintf(stdout,"%d logical Vorbis stream%s found in data.\n",
	    vorbiscount,(vorbiscount==1?"":"s"));
  
  fprintf(stdout,"Done.\n");
  
  ogg2_page_release(&og);
  ogg2_stream_destroy(os);
  ogg2_sync_destroy(oy);
  
  return 0;
}
  
