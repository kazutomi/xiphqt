// Simple 'detarkinizer'. Turn tarkin stream into a series of .pgm files
//
// Usage: nikrat [filename]
//
// $Id: tarkinplay.c,v 1.1 2001/02/13 01:06:24 giles Exp $
//
// $Log: tarkinplay.c,v $
// Revision 1.1  2001/02/13 01:06:24  giles
// Initial revision
//

#include <stdlib.h>
#include <SDL/SDL.h>
#include "bitwise.h"
#include "tarkin.h"

// Global Variables
tarkindata td;

// Function declarations
void dieusage();
void dumptofile(char *fn, char *data, tarkindata *td);
void make_palette(SDL_Surface *screen);

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
	SDL_Surface *screen;
	char *image;

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

	// Initialize video
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	screen = SDL_SetVideoMode(320, 240, 8, SDL_SWSURFACE);
	if (!screen) {
		printf("Unable to set 320x240 video: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_WM_SetCaption("tarkinplay v0.1beta", NULL);

	make_palette(screen);

	i = 0;
	while (!SDL_QuitRequested()) {
		printf("Showing frame #%i...\n", i);

		if (SDL_MUSTLOCK(screen)) {
			SDL_LockSurface(screen);
		}

		image = (char *)malloc(td.x_dim * td.y_dim);

		for (y = 0; y < td.y_dim; y++)
			for (x = 0; x < td.x_dim; x++)
				image[x + y * td.x_dim] = td.vectors[x + y * td.x_workspace + i * td.x_workspace * td.y_workspace];

		memcpy(screen->pixels, image, 320*240);

		free(image);

		if (SDL_MUSTLOCK(screen)) {
			SDL_UnlockSurface(screen);
		}

		SDL_UpdateRect(screen, 0, 0, 320, 240);

		SDL_Delay(100);

		i++;
		if (i > 31) i = 0;
	}

	// Shutdown
	free(td.vectors);
	free(td.dwtv);
	munmap(data, len);
	close(fd);
}

void dieusage()
{
	fprintf(stderr, "Usage:\ntarkinplay [filename]\n\n[filename] - Filename with compressed Tarkin stream\n");
	exit(1);
}

void make_palette(SDL_Surface *screen)
{
	int ncolors;
	int i;
	SDL_Color *palette;

	ncolors = screen->format->palette->ncolors;
	palette = (SDL_Color *)malloc(ncolors * sizeof(SDL_Color));
	for (i = 0; i < ncolors; i++) {
		palette[i].r = i;
		palette[i].g = i;
		palette[i].b = i;
	}
	SDL_SetColors(screen, palette, 0, ncolors);
}
