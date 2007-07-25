// Simple 'detarkinizer'. Turn tarkin stream into a series of .pgm files
//
// Usage: nikrat [filename]
//
// $Id: nikrat.c,v 1.1 2001/02/13 01:06:24 giles Exp $
//
// $Log: nikrat.c,v $
// Revision 1.1  2001/02/13 01:06:24  giles
// Initial revision
//

#include "tarkin.h"

// Global Variables
tarkindata td;

// Function declarations
void dieusage();
void dumptofile(char *fn, char *data, tarkindata *td);

int main(int argc, char **argv)
{
	FILE *fi;
	unsigned char ln[256], *data, *dp, *wksp;
	int a, d, fd, dims[3];
	int x, y, z;
	oggpack_buffer o;
	ulong len, lensub, l;
	struct stat st;
	long i;
	float f;

	if (argc < 2) 
		dieusage();

	if (!(fi = fopen(argv[1], "r"))) { 
		sprintf(ln, "Can't open %s", argv[1]); 
		perror(ln); 
		exit(1); 
	}

	fgets(ln, 255, fi);

	sscanf(ln, "%d %d %d", &td.x_dim, &td.y_dim, &td.z_dim);
	for (a = 0, d = 1; d < td.x_dim; a++, d <<= 1); 
	td.x_bits = a; 
	td.x_workspace = d;

	for (a = 0, d = 1; d < td.y_dim; a++, d <<= 1); 
	td.y_bits = a; 
	td.y_workspace = d;

	for (a = 0, d = 1; d < td.z_dim; a++, d <<= 1); 
	td.z_bits = a; 
	td.z_workspace = d;

	td.sz = td.x_dim * td.y_dim * td.z_dim;
	td.sz_workspace = td.x_workspace * td.y_workspace * td.z_workspace;
	fclose(fi);

	// Prepare bitstream for unpacking
	stat(argv[1], &st);
	len = st.st_size; 
	lensub = len;
	fd = open(argv[1], O_RDONLY);
	if ((data = mmap(0, len, PROT_READ, MAP_SHARED, fd, 0)) == (void*)-1) {
		close(fd);
		perror("Can't mmap file");
		exit(1);
	}
	for (dp = data; *dp != '\n' && lensub; dp++, lensub--);
	dp++; 
	lensub--;
	_oggpack_readinit(&o, dp, lensub);

	// Unpack bitstream
	unpackblock(&td, &o);

	// iDWT on bitstream
	dims[0] = td.x_workspace; dims[1] = td.y_workspace; dims[2] = td.z_workspace;
	dwt(td.vectors, dims, 3, -1);

	for (a = 0; a < td.sz_workspace; a++) { 
		if (td.vectors[a] < 0.0) 
			td.vectors[a] = 0.0;
		if (td.vectors[a] > 255.0) 
			td.vectors[a] = 255.0;
	}

	// Write bitplanes to .pgms
	wksp = (char *)malloc(td.x_dim * td.y_dim);
	for(z = 0; z < td.z_dim; z++) {
		sprintf(ln, "out_%d", z);
		for (y =0; y < td.y_dim; y++) {
			for (x = 0; x < td.x_dim; x++) {
				f = td.vectors[x + y * td.x_workspace + z * td.x_workspace * td.y_workspace];
				wksp[x + y * td.x_dim] = (unsigned char)f;
			}
		}
		dumptofile(ln, wksp, &td);
	}

	// Shutdown
	free(wksp);
	free(td.vectors);
	free(td.dwtv);
	munmap(data, len);
	close(fd);
}

void dumptofile(char *fn, char *data, tarkindata *td)
{
	FILE *fo;
	char tmp[256];
	int y;

	sprintf(tmp, "%s.pgm", fn);

	fo = fopen(tmp, "w");

	fprintf(fo, "P5\n%d %d\n255\n", td->x_dim, td->y_dim);
	for (y = 0; y < td->y_dim; y++) {
		fwrite(data + y * td->x_dim, td->x_dim, 1, fo);
	}

	fclose(fo);
}

void dieusage()
{
	fprintf(stderr, "Usage:\ndetark [filename]\n\n[filename] - Filename with compressed Tarkin stream\n");
	exit(1);
}
