/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 */

#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

#include <ogg/ogg.h>

#include "stream.h"
#include "output.h"
#include "common.h"

#define CHUNK_SIZE 4096

int unchain=1;
int ungroup=1;
char *outdir=NULL;

const char *broken_ogg = \
"  This Ogg file is broken and/or corrupted. This could be due to\n"
"  a flaw in the tool used to produce the file. If you believe this\n"
"  is the case, make sure to report it to the author/maintainer of\n"
"  that tool. Also note that as a consequence, some or all of the\n"
"  files procuduces by this invocation of OggSplit may also be\n"
"  invalid Ogg files.\n\n";

const char *optstring="cgo:uVh";
struct option options [] = {
  {"unchain", no_argument,       NULL, 'c'},
  {"ungroup", no_argument,       NULL, 'g'},
  {"outdir",  required_argument, NULL, 'o'},
  {"version", no_argument,       NULL, 'V'},
  {"help",    no_argument,       NULL, 'h'},
  {NULL,0,NULL,0}
};

static void usage(void)
{
  fprintf(stderr,
	  "Usage: oggsplit [options] input_files [...]\n"
	  "Supported options:\n"
	  "  -c --unchain   Only split chained streams. The default is to\n"
	  "                    split both grouped and chained streams.\n"
	  "  -g --ungroup   Only split grouped streams. This will cause only\n"
	  "                    the first link to be processed.\n"
	  "  -o --outdir <directory>\n"
	  "                 Create all output files in the specified directory.\n"
	  "  -V --version   Show OggSplit version.\n"
	  "  -h --help      Show summary of options.\n",
	  VERSION);
}

static int buffer_data(FILE *f, ogg_sync_state *oy)
{
  char *buffer=ogg_sync_buffer(oy, CHUNK_SIZE);
  int bytes=fread(buffer, 1, CHUNK_SIZE,f);
  ogg_sync_wrote(oy, bytes);
  return(bytes);
}

static int process_file(const char *pathname)
{
  int i;
  char *filename;

  FILE *infile;

  output_t *junk=NULL;

  stream_ctrl_t sc;
  output_ctrl_t oc;

  ogg_sync_state oy;
  ogg_page       og;

  int group_c=0; /* a group counter */
  int chain_c=0; /* a chain counter */
  int chain_state=0; /* 0 - nothing has passed through
		      * 1 - bos pages have passed through
		      * 2 - data pages have passed through
		      */

  infile=fopen(pathname, "r");
  if(infile==NULL){
    fprintf(stderr, "ERROR: Cannot open input file `%s': %s\n",
	    pathname, strerror(errno));
    return 0;
  }

  /* take out the path section from pathname */
  for(i=strlen(pathname)-1; i>=0; i--){
    if(pathname[i]=='/'){
      i++;
      break;
    }
  }

  if(outdir!=NULL){
    filename=xmalloc(strlen(outdir)+strlen(&pathname[i])+2);
    strcpy(filename, outdir);
    filename[strlen(outdir)]='/';
    strcpy(&filename[strlen(outdir)+1], &pathname[i]);
  }else{
    filename=xstrdup(&pathname[i]);    
  }

  stream_ctrl_init(&sc);
  output_ctrl_init(&oc, filename);

  ogg_sync_init(&oy);

  while(buffer_data(infile,&oy)){
    while(ogg_sync_pageout(&oy, &og)==1){
      stream_t *stream=NULL;

      if(ogg_page_bos(&og)){
	/* first page of stream */
	stream=stream_ctrl_stream_new(&sc, &og);

	switch(chain_state){
	case 2:
	  fprintf(stderr,
		  "WARNING: A logical stream began before previous streams had ended\n"
		  "  in `%s'.\n"
		  "  The previous logical streams may have been trunctated due to an\n"
		  "  incomplete transfer or an aborted encoding process.\n\n%s",
		  pathname, broken_ogg);
	  /* note the lack of break; */
	case 0:
	  /* don't open any new links if not unchaining */
	  if(!unchain && chain_c)
	    continue;

	  /* new chain link, create new output */
	  if(unchain)chain_c++;
	  if(ungroup)group_c++;
	  stream->op=output_ctrl_output_new(&oc, chain_c, group_c);
	  chain_state=1;
	  break;
	case 1:
	  if(ungroup){
	    stream->op=output_ctrl_output_new(&oc, chain_c, ++group_c);
	  }else{
	    /* use previously opened output */
	    stream->op=oc.outputs[oc.outputs_used-1];
	    stream->op->count++;
	  }
	  break;
	}

	printf("Ogg logical stream (serial %08x): type %s\n"
	       "  writing stream to `%s'\n\n",
	       stream->serial, stream_type_name(stream),
	       stream->op->filename);

      }else{
	/* test if this page belongs to an open stream */
	stream=stream_ctrl_stream_get(&sc, ogg_page_serialno(&og));

	if(stream==NULL){
	  /* there's junk in the file! */
	  if(junk==NULL){
	    /* call with 0, 0 to get the junk file */
	    junk=output_ctrl_output_new(&oc, 0, 0);
	    fprintf(stderr,
		    "WARNING: An Ogg stream (serial %08x) with no header was encountered\n"
		    "  in `%s'.\n"
		    "  All data from this stream and any further such streams will be written\n"
		    "  to `%s'.\n\n%s",
		    ogg_page_serialno(&og), filename,
		    junk->filename, broken_ogg);
	  }
	  output_page_write(junk, &og);
	  continue;
	}

	/* this wasn't junk */
	chain_state=2;
      }

      /* we have a stream, write it to output file */
      output_page_write(stream->op, &og);

      if(ogg_page_eos(&og)){
	/* last page of stream */
	output_ctrl_output_free(&oc, stream->op->id);
	stream_ctrl_stream_free(&sc, stream->serial);

	if(sc.streams_used==0){
	  if(!unchain)
	    /* don't go on to get another link (beware of the evil goto) */
	    goto doublebreak;

	  chain_state=0;
	  group_c=0;
	}
      }

    }
  }

 doublebreak:

  ogg_sync_clear(&oy);

  if(sc.streams_used)
    fprintf(stderr,
	    "WARNING: %d logical stream(s) remain unclosed at the end\n"
	    "  of `%s'.\n"
	    "  The logical streams may have been trunctated in an incomplete\n"
	    "  transfer or an aborted encoding process.\n\n%s",
	    sc.streams_used, filename, broken_ogg);

  free(filename);

  output_ctrl_free(&oc);
  stream_ctrl_free(&sc);

  fclose(infile);

  return 1;
}

int main(int argc, char **argv)
{
  DIR *test;

  int c;
  while((c=getopt_long(argc,argv,optstring,options,NULL))!=EOF){
    switch(c){
    case 'c':
      ungroup=0;
      break;
    case 'g':
      unchain=0;
      break;
    case 'o':
      if((test=opendir(optarg))==NULL){
	fprintf(stderr, "ERROR: Cannot use output directory `%s': %s\n",
		optarg, strerror(errno));
	exit(1);
      }
      closedir(test);

      if(outdir!=NULL)free(outdir);
      outdir=xstrdup(optarg);
      break;
    case 'V':
      printf("OggSplit %s\n", VERSION);
      exit(0);
      break;
    case 'h':
      usage();
      exit(0);
      break;
    case '?':
      fprintf(stderr, "Try `oggsplit --help' for more information.\n");
      exit(1);
    default:
      usage();
      exit(1);
    }
  }

  /* if no files given, bye bye bye */
  if(optind==argc){
    fprintf(stderr,
	    "oggsplit: no input files specified.\n"
	    "Try `oggsplit --help' for more information.\n");
    exit(1);
  }

  while(optind<argc){
    process_file(argv[optind++]);
    printf("\n");
  }

  if(outdir!=NULL)free(outdir);

  return 0;
}
