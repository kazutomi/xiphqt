#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "error.h"
#include "pnmio.h"
#include "image_ops.h"

char *progname = 0;
int verbosity = 0;

void usage();
void load_data(void *data, size_t size, size_t nobj, FILE *fp);

int
main(int argc, char **argv)
{
	int c;
	size_t width0,height0,type0,maxval0;
	size_t width1,height1,type1,maxval1;
	size_t image_size,pixel_size;

	char fname0[FILENAME_MAX] = "";
	char fname1[FILENAME_MAX] = "";
	
	FILE *fp0, *fp1;
	unsigned char *data0, *data1;
	double error;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "hv")) != EOF) {
		switch (c) {
		case 'v':
			verbosity++;
			break;
		case 'h':
		default:
			usage();
		}
	}

	if (argc - optind != 2)
		usage();
	
	strncpy(fname0, argv[optind], FILENAME_MAX);
	strncpy(fname1, argv[optind+1], FILENAME_MAX);

	if ((fp0=efopen(fname0,"r")) == NULL)
		exit(EXIT_FAILURE);

	if ((fp1=efopen(fname1,"r")) == NULL)
		exit(EXIT_FAILURE);
	
	
	if (!read_pnm_header(fp0, &type0, &width0, &height0, &maxval0))
		fatal("invalid pnm file in file %s", fname0);

	if (!read_pnm_header(fp1, &type1, &width1, &height1, &maxval1))
		fatal("invalid pnm file in file %s", fname1);
	
	if (type0 != type1)
		fatal("pnm files are not of the same type (%c,%c)", type0, type1);

	if (maxval0 != maxval1)
		fatal("pnm files are not of the same maxval (%d,%d)", maxval0, maxval1);
	
	if (width0 != width1)
		fatal("pnm files are not of the same width (%d,%d)", width0, width1);

	if (height0 != height1)
		fatal("pnm files are not of the same height (%d,%d)", height0, height1);

	/*if we get here it we have two valid files of the same type and size*/
	
	image_size = width0*height0;
	switch(type0){
	case PNM_RAW_GRAYSCALE:
		pixel_size = 1;
		break;
	case PNM_RAW_PIXMAP: 
		pixel_size = 3;
		break;
	default:
		fatal("unable to handle pnm type %d", type0);
	}
	
	data0 = (unsigned char *) emalloc(pixel_size*image_size);
	data1 = (unsigned char *) emalloc(pixel_size*image_size);

	load_data(data0,pixel_size,image_size,fp0);
	load_data(data1,pixel_size,image_size,fp1);

	error = psnr(data0,data1,pixel_size*image_size,maxval0);
	switch(verbosity){
	case 0:
		printf("%g",error);
		break;
	case 1:
		printf("psnr is %g\n",error);
		break;
	case 2:
	default:
		printf("psnr between %s and %s is %g\n",fname0, fname1,error);
		break;				
	}
	return 0;
}

void 
usage()
{
	fprintf(stderr,"usage: %s [-vh] file1 file2\n",progname);
	exit(EXIT_FAILURE);
}

void
load_data(void *data, size_t size, size_t nobj, FILE *fp)
{
	size_t len=fread(data,size,nobj,fp);
//	if (fread(data,size,nobj,fp) != nobj){
	if (len != nobj){
		if ( feof(fp))
			fatal("premature eof in file");
		if (ferror(fp))
			perror("loading pgm");
		fatal("failed to load file");
	}
}
