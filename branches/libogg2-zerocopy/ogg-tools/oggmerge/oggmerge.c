/*
oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	oggmerge.c

	Copyright 2000 Ralph Giles <Ralph_Giles@telus.net>
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

#include "oggmerge.h"
#include "vorbis.h"
#include "midi.h"
#include "mng.h"

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
		"         -h, --help     this help\n"
		"         -v, --verbose  verbose status\n"
		"         -q, --quiet    suppress status output\n"
		"         --version      print version information\n"
	);
}

static void _set_defaults(void)
{
	/* set defaults */
	params.outfile = NULL;
	params.out = NULL;
	params.input = NULL;
	params.verbose = 1;
}

struct option long_options[] = {
	{"quiet", 0, NULL, 'q'},
	{"verbose", 0, NULL, 'v'},
	{"help", 0, NULL, 'h'},
	{"help", 0, NULL, '?'},
	{"version", 0, NULL, 'V'},
	{"output", 1, NULL, 'o'},
	{NULL, 0, NULL, 0}
};

static void _parse_args(int argc, char **argv)
{
	int ret;
	int option_index = 1;

	while ((ret = getopt_long(argc, argv, "qvh?Vo:", long_options, &option_index)) != -1) {
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

	if (strcasecmp(ext, ".ogg") == 0)
		return TYPEVORBIS;
	else if (strcasecmp(ext, ".mid") == 0)
		return TYPEMIDI;
	else if (strcasecmp(ext, ".mng") == 0)
		return TYPEMNG;
	return TYPEUNKNOWN;
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

		file->fp = NULL;
		switch (file->type) {
		case TYPEVORBIS:
			file->state_init = vorbis_state_init;
			file->data_in = vorbis_data_in;
			file->page_out = vorbis_page_out;
			break;
		case TYPEMIDI:
			file->state_init = midi_state_init;
			file->data_in = midi_data_in;
			file->page_out = midi_page_out;
			break;
		case TYPEMNG:
			file->state_init = mng_state_init;
			file->data_in = mng_data_in;
			file->page_out = mng_page_out;
		}

		file->serialno = 0;
		file->status = EMOREDATA;
		file->page = NULL;
		file->next = NULL;

		_add_file(file);
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
		fprintf(stderr, "Opening %s for reading...\n", file->name);
		file->fp = fopen(file->name, "r");
		if (file->fp == NULL) {
			fprintf(stderr, "Error: Couldn't open input file %s.\n", file->name);
			return 1;
		}
		
		do {
			serialno = rand();
		} while (!_unique_serialno(serialno));
		file->serialno = serialno;

		file->state_init(&file->state, file->serialno);

		file = file->next;
	}

	/* let her rip! */
	while (1) {
		/* Step 1: make sure an ogg page is available for each input 
		** as long we havne't already processed the last one
		*/
		file = params.input;
		while (file) {
			if (file->page == NULL) {
				while ((file->page = file->page_out(&file->state)) == NULL && file->status == EMOREDATA) {
					if (feof(file->fp)) {
						file->status = 0;
						break;
					}
					bytes = fread(buf, 1, BUFSIZE, file->fp);
					file->status = file->data_in(&file->state, buf, bytes);
					if (file->status < 0 && file->status != EMOREDATA) {
						fprintf(stderr, "Error: Packetizer error on file %s.\n", file->name);
						return 1;
					}
				}
			}

			file = file->next;
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
		fclose(file->fp);
		file = file->next;
	}
	fclose(params.out);

	return 0;
}



