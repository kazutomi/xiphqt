#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <values.h>

#include "error.h"
#include "pnmio.h"
#include "image_ops.h"

char *progname = 0;
int verbosity = 0;

void usage();
int valid_fmt(char *p);
int check_copy(char *fname, const char *fmt, size_t frame);
void load_data(void *data, size_t size, size_t nobj, FILE *fp);


int
main(int argc, char **argv)
{
	int c;
	unsigned long int num_frames = ULONG_MAX;
	unsigned long int initial_frame = 0;
	unsigned long int frame;

	int collect_stats=0;
	double max_psnr=0, min_psnr=DBL_MAX, avg_psnr=0;
	unsigned long int max_frame=0, min_frame=0;

	size_t width,height,type,maxval;
	size_t image_size,pixel_size;
	
	char fmt0[FILENAME_MAX+1] = "";
	char fmt1[FILENAME_MAX+1] = "";
	char fname0[FILENAME_MAX+1] = "";
	char fname1[FILENAME_MAX+1] = "";
	
	FILE *fp0, *fp1;
	unsigned char *data0, *data1;
	double curr_psnr;

	progname = argv[0];
	
	while ((c = getopt(argc, argv, "hvsi:n:")) != EOF) {
		switch (c) {
		case 'v':
			verbosity++;
			break;
		case 'i':
			initial_frame = strtoul(optarg,(char**)NULL,10);
			break;
		case 'n':
			num_frames = strtoul(optarg,(char**)NULL,10);
			break;			
		case 's':
			collect_stats = 1;
			break;
		case 'h':
		default:
			usage();
		}
	}

	if (argc - optind != 2)
		usage();

	
	strncpy(fmt0,argv[optind],FILENAME_MAX);
	strncpy(fmt1,argv[optind+1],FILENAME_MAX);

	if (!valid_fmt(fmt0))
		fatal("invalid format: %s", fmt0);

	if (!valid_fmt(fmt1))
		fatal("invalid format: %s", fmt1);
	
	if (!check_copy(fname0,fmt0,0))
		fatal("filename size exceeded");

	if ((fp0=efopen(fname0,"r")) == NULL)
		exit(EXIT_FAILURE);
	
	if (!read_pnm_header(fp0, &type, &width, &height, &maxval))
		fatal("invalid pnm file in file %s", fname0);

	if (maxval != 255)
		fatal("unable to handle pnm maxval %d", maxval);

	image_size = width*height;
	switch(type){
	case PNM_RAW_GRAYSCALE:
		pixel_size = 1;
		break;
	case PNM_RAW_PIXMAP: 
		pixel_size = 3;
		break;
	default:
		fatal("unable to handle pnm type %d", type);
	}
	
	data0 = (unsigned char *) emalloc(pixel_size*image_size);
	data1 = (unsigned char *) emalloc(pixel_size*image_size);
	fclose(fp0);

	for (frame = initial_frame; frame < initial_frame+num_frames;++frame){
		size_t width0,height0,type0,maxval0;
		size_t width1,height1,type1,maxval1;

		if (!check_copy(fname0,fmt0,frame))
			fatal("filename size exceeded");

		if (!check_copy(fname1,fmt1,frame))
			fatal("filename size exceeded");

		if ((fp0=efopen(fname0,"r")) == NULL ||
			(fp1=efopen(fname1,"r")) == NULL )
			break;
		
		if (!read_pnm_header(fp0, &type0, &width0, &height0, &maxval0))
			fatal("invalid pnm file in file %s", fname0);
		if (!read_pnm_header(fp1, &type1, &width1, &height1, &maxval1))
			fatal("invalid pnm file in file %s", fname1);

		if (type0 != type || width0 != width || height0 != height || maxval0 != maxval)
			fatal("pnm file %s has incorrect shape or type", fname0);
		if (type1 != type || width1 != width || height1 != height || maxval1 != maxval)
			fatal("pnm file %s has incorrect shape or type", fname1);
		
		load_data(data0,pixel_size,image_size,fp0);
		load_data(data1,pixel_size,image_size,fp1);
		
		curr_psnr = psnr(data0,data1,pixel_size*image_size,maxval);
		
		switch(verbosity){
		case 0:
			printf("%g\n",curr_psnr);
			break;
		case 1:
			printf("psnr is %g\n",curr_psnr);
			break;
		case 2:
		default:
			printf("psnr between %s and %s is %g\n",fname0, fname1,curr_psnr);
			break;				
		}
		
		if (collect_stats) {
			avg_psnr+=curr_psnr;
			if (curr_psnr > max_psnr){
				max_psnr=curr_psnr;
				max_frame=frame;
			} else if (curr_psnr < min_psnr){
				min_psnr=curr_psnr;
				min_frame=frame;
			}
		}

	} /* loop over frames */
	
	if (collect_stats) {
		num_frames = frame - 1 - initial_frame; /* cover case when num_frames not specified */
		avg_psnr/=num_frames;
		
		printf ("Number of frames: %lu\n"
				"Average PSNR: %g\n"
				"Best PSNR: %g Frame: %lu\n"
				"Worst PSNR %g Frame: %lu\n", frame, avg_psnr, max_psnr, max_frame, min_psnr, min_frame);
	}

	free(data0); 
	free(data1); 
	return 0;
}

int
valid_fmt(char *p)
{
	int found_percent=0;
	
	do{
		if (*p == '%'){
			if (found_percent)
				return 0;
			else
				found_percent=1;
			++p;
			switch(*p){
			case 'i': /* only accept %i */
				break;
			default:
				return 0;
			}		
		}			
	} while (*p++ != '\0');
	
	return 1;
}

int
check_copy(char *fname, const char *fmt, size_t frame)
{
	if ( (strlen(fmt) + (unsigned long int) log10(frame) + 1 ) > FILENAME_MAX)
		return 0;

	if (sprintf(fname,fmt,frame) < 0)
		return 0;

	return 1;
}

void 
usage()
{
	fprintf(stderr,"usage: %s [-vhs] [-n num frames] [-i initial frame] template1 template2\n",progname);
	fprintf(stderr,"\t -s: generate statistics\n");
	fprintf(stderr,"\t -v: add verbosity\n");
	fprintf(stderr,"\t template: foo%%i.ppm\n");
	exit(EXIT_FAILURE);
}

void
load_data(void *data, size_t size, size_t nobj, FILE *fp)
{
	if (fread(data,size,nobj,fp) != nobj){
		if ( feof(fp))
			fatal("premature eof in file");
		if (ferror(fp))
			perror("loading pgm");
		fatal("failed to load file");
	}
}







