/*
	oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	oggmerge.c

	Copyright 2000 Ralph Giles <Ralph_Giles@telus.net>

	Distributed under the GPL
	see http://www.gnu.org/copyleft/gpl.html for details
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ogg/ogg.h>

#include "oggmerge.h"
#include "config.h"

void usage(void)
{
	/* prefer stdout to stderr to coddle win32 */
	FILE *out = stdout;

	fprintf(out,
		"oggmerge [-o <outfile>] <file1> [<file2> ...]\n"
		"  Any of the file arguments can be '-' for stdin/out\n"
		"  Other options:\n"
		"         -h             this help\n"
		"         --help         longer help\n"
		"         -v             verbose status\n"
		"         -q, --quiet    suppress status output\n"
		"         --version      print version information\n"
		"  caveat: much of this is unimplemented!\n"
	);
}

int main(int argc, char *argv[])
{
	char *infile,*outfile;
	FILE *in,*out;
	param_t *param;

	if (argc != 2) {
		usage();
		exit(1);
	}

	/* initialize defaults */
	param = malloc(sizeof(*param));
	if (param == NULL) {
		fprintf(stderr, "couldn't allocation memory for parameters\n");
		exit(1);
	}
	param->in = param->out = NULL;
	param->infile = param->outfile = NULL;
	param->quiet = 0;
	param->verbose = 0;
	param->os = NULL;
	param->oy = NULL;

	infile = strdup(argv[1]);
	in = fopen(infile, "rb");
	if (in == NULL) {
		fprintf(stderr, "Could not open '%s'\n", infile);
		exit(1);
	}

	if (strlen(infile)<5) {
		outfile = "out.ogg";
	} else {
		outfile = strdup(infile);
		outfile[strlen(infile)-3] = 'o';
		outfile[strlen(infile)-2] = 'g';
		outfile[strlen(infile)-1] = 'g';
		outfile[strlen(infile)] = '\0';
	}
	out = fopen(outfile, "wb");
	if (out == NULL) {
		fprintf(stderr, "Could not open '%s'\n", outfile);
                exit(1);    
        }
	fprintf(stderr, "converting %s to %s\n", argv[1], outfile);

	/* fill in parameters for the conversion */
	param->in = in;
	param->out = out;
	param->infile = infile;
	param->outfile = outfile;

	/* set up the ogg stream for output */
	param->os = (ogg_stream_state*)malloc(sizeof(ogg_stream_state));
	param->oy = (ogg_sync_state*)malloc(sizeof(ogg_sync_state));
	ogg_stream_init(param->os,0x1f1f3e3e);
	ogg_sync_init(param->oy);

	mngconvert(param);

	fclose(param->out);
	fclose(param->in);

	free(param);
}
