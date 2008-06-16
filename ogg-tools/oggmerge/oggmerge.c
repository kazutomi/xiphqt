/*
oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	oggmerge.c

	Copyright 2000 Ralph Giles <giles@xiph.org>
	               Jack Moffitt <jack@xiph.org>

	Distributed under the GPL
	see http://www.gnu.org/copyleft/gpl.html for details
*/
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <ogg/ogg.h>

#include "config.h"
#include "oggmerge.h"
#include "vorbis.h"
#include "midi.h"
#include "mng.h"
#include "kate.h"
#include "theora.h"
#include "speex.h"
#include "skeleton.h"

#define VERSIONINFO "oggmerge v" VERSION "\n"

param_t params;

static void _usage(void)
{
	/* prefer stdout to stderr to coddle win32 */
	FILE *out = stdout;

	fprintf(out,
		VERSIONINFO \
		"oggmerge [-o <outfile> | --output=<outfile>] <file1> <file2> [<file3> ...]\n"
		"  Any of the file arguments can be '-' for stdin/out\n"
		"  Other options:\n"
		"         -h, --help          this help\n"
		"         -v, --verbose       verbose status\n"
		"         -q, --quiet         suppress status output\n"
		"         -O, --old-style     use old style sorting method (use previous packet's granulepos)\n"
		"         -s, --skeleton      adds a Skeleton track to the output\n"
		"         --version           print version information\n"
	);
}

static void _set_defaults(void)
{
	/* set defaults */
	params.outfile = NULL;
	params.out = NULL;
	params.input = NULL;
	params.verbose = 1;
        params.old_style = 0;
        params.skeleton = 0;
}

struct option long_options[] = {
	{"quiet", 0, NULL, 'q'},
	{"verbose", 0, NULL, 'v'},
	{"help", 0, NULL, 'h'},
	{"help", 0, NULL, '?'},
	{"version", 0, NULL, 'V'},
	{"output", 1, NULL, 'o'},
	{"old-style", 0, NULL, 'O'},
	{"skeleton", 0, NULL, 's'},
	{NULL, 0, NULL, 0}
};

static void _parse_args(int argc, char **argv)
{
	int ret;
	int option_index = 1;

	while ((ret = getopt_long(argc, argv, "qvh?VOso:", long_options, &option_index)) != -1) {
		switch (ret) {
		case 0:
			fprintf(stderr, "Internal error parsing command line options.\n");
			exit(1);
			break;
		case 'q':
			/* default is 1. quiet is 0, verbose is > 1 */
			params.verbose = 0;
			break;
		case 'v':
			params.verbose = 2;
			break;
		case '?':
		case 'h':
			_usage();
			exit(0);
			break;
		case 'V':
			fprintf(stderr, VERSIONINFO);
			exit(0);
			break;
		case 'o':
			if (params.outfile != NULL) {
				fprintf(stderr, "Only one output file allowed.\n");
				exit(1);
			}
			params.outfile = (char *)strdup(optarg);
			break;
                case 'O':
                        params.old_style = 1;
                        break;
                case 's':
                        params.skeleton = 1;
                        break;
		default:
			_usage();
			exit(0);
		}
	}
}

static int _get_type(char *filename)
{
	char *ext;

	ext = strrchr(filename, '.');
	if (ext == NULL) return TYPEUNKNOWN;

	/* should be smarter and check file magic */
	/* yes, it would, wouldn't it ? let's do the bare minimum though :) */

        /* simpler than adding the list of full extensions, esp if others in og[^gvax] start to be used too */
	if (strncasecmp(ext, ".og",3) == 0 && !ext[4] && strchr("gGvVaAxX",ext[3])) {
                int offset=28; /* 27 for header, 1 for small packet lacing */
                char buf[64];
                FILE *f=fopen(filename,"r");
                if (!f) return TYPEUNKNOWN;
                memset(buf,0,sizeof(buf)); /* avoid looking at stale data for small files */
                fread(buf,1,sizeof(buf),f);
                fclose(f);
                if (!memcmp(buf+offset,"\001vorbis",7)) return TYPEVORBIS;
                if (!memcmp(buf+offset,"\200kate\0\0\0",8)) return TYPEKATE;
                if (!memcmp(buf+offset,"\200theora",7)) return TYPETHEORA;
                if (!memcmp(buf+offset,"Speex   ",8)) return TYPESPEEX;
                return TYPEUNKNOWN;
        }
	else if (strcasecmp(ext, ".mid") == 0)
		return TYPEMIDI;
	else if (strcasecmp(ext, ".mng") == 0)
		return TYPEMNG;
	else if (strcasecmp(ext, ".spx") == 0)
		return TYPESPEEX;
	return TYPEUNKNOWN;
}

static const char *_get_file_type_name(int type)
{
  switch (type) {
    default: return "Er, not sure, code's broken";
    case TYPESKELETON: return "Skeleton";
    case TYPEUNKNOWN: return "Unknown";
    case TYPEVORBIS: return "Vorbis";
    case TYPEMIDI: return "MIDI";
    case TYPEMNG: return "MNG";
    case TYPEKATE: return "Kate";
    case TYPETHEORA: return "Theora";
    case TYPESPEEX: return "Speex";
  }
}

static void _add_file(filelist_t *file)
{
	filelist_t *temp;

	if (params.input == NULL) {
		params.input = file;
	} else {
		temp = params.input;
		while (temp->next) temp = temp->next;
		temp->next = file;
	}
}

static void _prepend_file(filelist_t *file)
{
        file->next = params.input;
	params.input = file;
}

static int _unique_serialno(int serialno)
{
	filelist_t *file;

	file = params.input;
	while (file) {
		if (file->serialno == serialno) return 0;
		file = file->next;
	}
	return 1;
}

/* copied and adapted from ffmpeg2theora (GPL too) */
#define SKELETON_VERSION_MAJOR 3
#define SKELETON_VERSION_MINOR 0
#define FISHEAD_IDENTIFIER "fishead\0"
#define FISBONE_IDENTIFIER "fisbone\0"
#define FISBONE_SIZE 52
#define FISBONE_MESSAGE_HEADER_OFFSET 44

static void write16le(unsigned char *ptr,ogg_uint16_t v)
{
  ptr[0]=v&0xff;
  ptr[1]=(v>>8)&0xff;
}

static void write32le(unsigned char *ptr,ogg_uint32_t v)
{
  ptr[0]=v&0xff;
  ptr[1]=(v>>8)&0xff;
  ptr[2]=(v>>16)&0xff;
  ptr[3]=(v>>24)&0xff;
}

static void write64le(unsigned char *ptr,ogg_int64_t v)
{
  ogg_uint32_t hi=v>>32;
  ptr[0]=v&0xff;
  ptr[1]=(v>>8)&0xff;
  ptr[2]=(v>>16)&0xff;
  ptr[3]=(v>>24)&0xff;
  ptr[4]=hi&0xff;
  ptr[5]=(hi>>8)&0xff;
  ptr[6]=(hi>>16)&0xff;
  ptr[7]=(hi>>24)&0xff;
}

static void _add_fishead_packet (ogg_packet *op) {

    if (!op) return;
    memset (op, 0, sizeof (*op));

    op->packet = _ogg_calloc (64, sizeof(unsigned char));
    if (op->packet == NULL) return;

    memset (op->packet, 0, 64);
    memcpy (op->packet, FISHEAD_IDENTIFIER, 8); /* identifier */
    write16le(op->packet+8, SKELETON_VERSION_MAJOR); /* version major */
    write16le(op->packet+10, SKELETON_VERSION_MINOR); /* version minor */
    write64le(op->packet+12, (ogg_int64_t)0); /* presentationtime numerator */
    write64le(op->packet+20, (ogg_int64_t)1000); /* presentationtime denominator */
    write64le(op->packet+28, (ogg_int64_t)0); /* basetime numerator */
    write64le(op->packet+36, (ogg_int64_t)1000); /* basetime denominator */
    /* both the numerator are zero hence handled by the memset */
    write32le(op->packet+44, 0); /* UTC time, set to zero for now */

    op->b_o_s = 1; /* its the first packet of the stream */
    op->e_o_s = 0; /* its not the last packet of the stream */
    op->bytes = 64; /* length of the packet in bytes */
}

void add_fisbone_packet (ogg_packet *op,
                         ogg_uint32_t serial,
                         const char *content_type, int headers, int preroll,
                         int gshift, ogg_int64_t gnum, ogg_int64_t gden)
{
    int n;
    size_t ctlen;
    size_t plen;

    if (!content_type) return;

    if (params.verbose>=2)
        printf("adding fisbone for %s\n",content_type);

    ctlen = strlen(content_type);
    plen = FISBONE_SIZE+16+ctlen; /* 16 is strlen "Content-Type: \r\n" */

    memset (op, 0, sizeof (*op));
    op->packet = _ogg_calloc (plen, sizeof(unsigned char));
    if (op->packet == NULL) return;

    memset (op->packet, 0, plen);
    /* it will be the fisbone packet for the theora video */
    memcpy (op->packet, FISBONE_IDENTIFIER, 8); /* identifier */
    write32le(op->packet+8, FISBONE_MESSAGE_HEADER_OFFSET); /* offset of the message header fields */
    write32le(op->packet+12, serial); /* serialno of the theora stream */
    write32le(op->packet+16, headers); /* number of header packets */
    /* granulerate, temporal resolution of the bitstream in samples/microsecond */
    write64le(op->packet+20, gnum); /* granulrate numerator */
    write64le(op->packet+28, gden); /* granulrate denominator */
    write64le(op->packet+36, 0); /* start granule */
    write32le(op->packet+44, preroll); /* preroll, for theora its 0 */
    *(op->packet+48) = gshift; /* granule shift */
    sprintf(op->packet+FISBONE_SIZE, "Content-Type: %s\r\n", content_type); /* message header field, Content-Type */

    op->b_o_s = 0;
    op->e_o_s = 0;
    op->bytes = plen; /* size of the packet in bytes */
}

static void _add_fishtail_packet(ogg_packet *op)
{
    if (!op) return;

    /* build and add the e_o_s packet */
    memset (op, 0, sizeof (*op));
    op->b_o_s = 0;
    op->e_o_s = 1; /* its the e_o_s packet */
    op->granulepos = 0;
    op->bytes = 0; /* e_o_s packet is an empty packet */
}

static void _fill_filelist(filelist_t *file)
{
    file->fp = NULL;
    switch (file->type) {
    case TYPESKELETON:
            file->state_init = skeleton_state_init;
            file->data_in = skeleton_data_in;
            file->page_out = skeleton_page_out;
            file->fisbone_out = skeleton_fisbone_out;
            break;
    case TYPEVORBIS:
            file->state_init = vorbis_state_init;
            file->data_in = vorbis_data_in;
            file->page_out = vorbis_page_out;
            file->fisbone_out = vorbis_fisbone_out;
            break;
    case TYPEKATE:
            file->state_init = kate_state_init;
            file->data_in = kate_data_in;
            file->page_out = kate_page_out;
            file->fisbone_out = kate_fisbone_out;
            break;
    case TYPESPEEX:
            file->state_init = speex_state_init;
            file->data_in = speex_data_in;
            file->page_out = speex_page_out;
            file->fisbone_out = speex_fisbone_out;
            break;
    case TYPETHEORA:
            file->state_init = theora_state_init;
            file->data_in = theora_data_in;
            file->page_out = theora_page_out;
            file->fisbone_out = theora_fisbone_out;
            break;
    case TYPEMIDI:
            file->state_init = midi_state_init;
            file->data_in = midi_data_in;
            file->page_out = midi_page_out;
            file->fisbone_out = midi_fisbone_out;
            break;
    case TYPEMNG:
            file->state_init = mng_state_init;
            file->data_in = mng_data_in;
            file->page_out = mng_page_out;
            file->fisbone_out = mng_fisbone_out;
            break;
    }

    file->serialno = 0;
    file->status = EMOREDATA;
    file->page = NULL;
    file->next = NULL;
    file->fisbone_done = 0;
}

#define BUFSIZE 1024

int main(int argc, char **argv)
{
	char **infiles;
	int numfiles;
	int i;
	filelist_t *file, *winner;
	unsigned long bytes;
	int serialno;
	int first_pages = 1;
	ogg_page *page;
	int fishtail_done = 0;
	char buf[BUFSIZE];

	srand(time(NULL));

	_set_defaults();
	_parse_args(argc, argv);
	
	if (optind >= argc) {
		fprintf(stderr, "Error: No input files specified.  Use -h for help.\n");
		return 1;
	} else {
		infiles = argv + optind;
		numfiles = argc - optind;
	}

	/* setup the input files */
	for (i = 0; i < numfiles; i++) {
		file = (filelist_t *)malloc(sizeof(filelist_t));
		if (file == NULL) {
			fprintf(stderr, "Error: Memory error.\n");
			return 1;
		}

		file->name = (char *)strdup(infiles[i]);
		file->type = _get_type(file->name);
		
		if (file->type == TYPEUNKNOWN) {
			fprintf(stderr, "Error: File %s has unknown type.\n", file->name);
			return 1;
		}
                else {
                  if (params.verbose>=1) printf("%s: %s\n",file->name,_get_file_type_name(file->type));
                }


		_fill_filelist(file);

		_add_file(file);
	}

        /* If skeleton is requested, add a new stream for it at the start */
        if (params.skeleton) {
		file = (filelist_t *)malloc(sizeof(filelist_t));
		if (file == NULL) {
			fprintf(stderr, "Error: Memory error.\n");
			return 1;
		}

		file->name = NULL;
		file->type = TYPESKELETON;

                _fill_filelist(file);

                _prepend_file(file);
        }

	/* open output file */
	if (params.outfile == NULL) {
		params.outfile = (char *)strdup("stdout");
		params.out = stdout;
	} else {
		params.out = fopen(params.outfile, "w");
		if (params.out == NULL) {
			fprintf(stderr, "Error: Couldn't open output file %s.\n", params.outfile);
			return 1;
		}
	}

	/* open all files and prepare for processing */
	file = params.input;
	while (file) {
                if (file->type != TYPESKELETON) {
                    if (params.verbose>=2) printf("Opening %s for reading...\n", file->name);
                    file->fp = fopen(file->name, "r");
                    if (file->fp == NULL) {
                          fprintf(stderr, "Error: Couldn't open input file %s.\n", file->name);
                         return 1;
                    }
		}
		
		do {
			serialno = rand();
		} while (!_unique_serialno(serialno));
		file->serialno = serialno;

		file->state_init(&file->state, file->serialno, params.old_style);

		file = file->next;
	}

        /* a fishead first if skeleton is required */
        if (params.skeleton) {
            ogg_packet op;
            _add_fishead_packet(&op);
            skeleton_packetin(&params.input->state, &op, FISHEAD);
            _ogg_free (op.packet);
        }

	/* let her rip! */
	while (1) {
		/* Step 1: make sure an ogg page is available for each input 
		** as long we haven't already processed the last one
		*/
		file = params.input;
		while (file) {
			if (file->page == NULL) {
				while ((file->page = file->page_out(&file->state)) == NULL && file->status == EMOREDATA) {
                                    bytes = 0;
                                    if (file->type != TYPESKELETON) {
					    if (feof(file->fp)) {
						    file->status = 0;
						    break;
					    }
					    bytes = fread(buf, 1, BUFSIZE, file->fp);
                                    }
				    file->status = file->data_in(&file->state, buf, bytes);
                                    if (file->type != TYPESKELETON) {
				        if (file->status < 0 && file->status != EMOREDATA) {
					    fprintf(stderr, "Error: Packetizer error on file %s.\n", file->name);
					    return 1;
                                        }
				    }
				}
                                /* now that we have at least the BOS packet, we can get a skeleton packet is required */
                                if (params.skeleton && !file->fisbone_done) {
                                        ogg_packet op;
                                        if (!file->fisbone_out(&file->state, &op)) {
                                              skeleton_packetin(&params.input->state, &op, FISBONE);
                                              _ogg_free (op.packet);
                                              file->fisbone_done = 1;
                                        }
                                }
			}

			file = file->next;
		}

                /* a fishtail last, but before any data packet, if skeleton is required */
                if (params.skeleton && !fishtail_done) {
                    ogg_packet op;
                    _add_fishtail_packet(&op);
                    skeleton_packetin(&params.input->state, &op, FISHTAIL);
                    _ogg_free (op.packet);
                    fishtail_done = 1;
                }

		/* Step 1.5: Write out the first page of each stream
		** because headers must come together before any
		** non-header pages.
		*/
		if (first_pages) {
			first_pages = 0;
			file = params.input;
			while (file) {
				if (file->page == NULL) {
					fprintf(stderr, "Error: File %s didn't produce a header page.\n", file->name);
					return 1;
				}
                                if (params.verbose>=2) printf("writing header page for %08x: %s\n",file->serialno,file->name);

				page = file->page->og;

				bytes = fwrite(page->header, 1, page->header_len, params.out);
				if (bytes != page->header_len) {
					fprintf(stderr, "Error: Output error writing to %s.\n", params.outfile);
					return 1;
				}
				bytes = fwrite(page->body, 1, page->body_len, params.out);
				if (bytes != page->body_len) {
					fprintf(stderr, "Error: Output error writing to %s.\n", params.outfile);
					return 1;
				}
			
				free(page->header);
				free(page->body);
				free(page);
				free(file->page);
				file->page = NULL;

				file = file->next;
			}

			continue;
		}

		/* Step 2: Pick the page with the lowest timestamp and 
		** stuff it into the ogg stream
		*/
		winner = params.input;
		file = winner->next;
		while (file) {
			if (file->page != NULL) {
				if (winner->page != NULL) {
					if (file->page->timestamp < winner->page->timestamp)
						winner = file;
				} else {
					winner = file;
				}
			}
			file = file->next;
		}

		/* exit if there are no more pages */
		if (winner->page == NULL) break;

		/* Step 3: Write out the winning page */
                if (params.verbose>=2)
                  printf("writing winning page (%s, timestamp %lld)\n",winner->name,winner->page->timestamp);
		page = winner->page->og;

		bytes = fwrite(page->header, 1, page->header_len, params.out);
		if (bytes != page->header_len) {
			fprintf(stderr, "Error: Output error writing to %s.\n", params.outfile);
			return 1;
		}
		bytes = fwrite(page->body, 1, page->body_len, params.out);
		if (bytes != page->body_len) {
			fprintf(stderr, "Error: Output error writing to %s.\n", params.outfile);
			return 1;
		}

		/* Step 4: Cleanup! */
		free(page->header);
		free(page->body);
		free(page);
		free(winner->page);
		winner->page = NULL;
	}

	file = params.input;
	while (file) {
                if (file->fp)
                    fclose(file->fp);
		file = file->next;
	}
	fclose(params.out);

	return 0;
}
