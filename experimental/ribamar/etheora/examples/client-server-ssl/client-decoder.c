/*Copyright, 2007, by Ribamar Santarosa, ribamar@gmail.com */

/*
  a bit more sofisticated theora video decoder using etheora. this 
  example is an client which reads data from a (encoder) server. data
  is read from the server though an SSL layer. Video is outputted 
  into an SDL window. 

  note this doesn't have a good performance yet and can't deal with 
  the real fps rate. 

  usage: 
  ./client-decoder [file.ogv] 
  file.ogv: file to read video input. if none is passed, data is read
  from standard input.

  build command line example (with gcc): 
  gcc client-decoder.c path/to/etheora.c  -I/path/to/etheora.h/dir \
  		-ltheora -lssl -lSDL -o client-decoder

*/

#include <etheora.h>
#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>


int main(int argc, char **args){
	int i, j; 
	etheora_ctx ec; 
	FILE *finfo, *fin; 
	float r, g, b; 
	SDL_Surface *scr; 
	SDL_Event sev; 

	/* opening video input file and file to print debuf info. as 
	   always, sockets could be used. Degub.txt can have a fast 
	   increase, you may want open a null device file as 
	   /dev/null. */
	finfo = fopen("Debug.txt", "w"); 
	if(argc > 1) {
		fin = fopen(args[1], "r"); 
	}
	else {
		fin = stdin; 
		fprintf(stderr, "opening standard input as video file. \n"); 
	}
	if(fin == NULL || finfo == NULL){
		fprintf(stderr, "Debug.txt or input file couldn't be open.\n"); 
		return 1; 
	}
	fprintf(stderr, "debug info in Debug.txt.\n"); 

	/* configuring decoder. */
	etheora_dec_setup(&ec, fin, finfo);

	/* start the decoder. */
	if (etheora_dec_start(&ec)){
		fprintf(stderr, "Can't start decoding.\n"); 
		return 2; 
	}

	/*sdl stuff - setting video operation. */
	scr = SDL_SetVideoMode(etheora_get_width(&ec), etheora_get_height(&ec),
		16, SDL_SWSURFACE);
		/*8, SDL_SWSURFACE);*/
	if (scr == NULL){
		fprintf(stderr, "Can't start video: %s.\n",  SDL_GetError()); 
		return 3; 
	}
	fprintf(stderr, "Bytes per pixel: %i.\n", scr->format->BytesPerPixel); 


	/*getting next frame from decoder or a window quit act.*/
	while( !etheora_dec_nextframe(&ec) && sev.type != SDL_QUIT ){

		/*sdl stuff - locking the screen so we can draw in the sdl
		 * memory without changes appearing in  the screen. */
		if(SDL_MUSTLOCK(scr)) SDL_LockSurface(scr); 
		SDL_WaitEvent(&sev); 

		/*now we can read the frame buffer data. */
		for( i = 0; i < etheora_get_width(&ec); i++)
			for( j = 0; j < etheora_get_height(&ec); j++){
				/* or use etheora_dec_yuv draw() to 
			           read in yuv colorspace.*/
				etheora_dec_rgb_read(&ec, i, j, 
					 &r, &g, &b);  

				/* HERE YOU MAY DO WHAT YOU WANT
				  WITH THE PIXEL. */
				  /*
				  *((Uint8 *)scr->pixels + j*scr->pitch/1 + i) = 
				  SDL_MapRGB(scr->format, (Uint8)r, (Uint8)g, (Uint8)b); 
				  */
				  *((Uint16 *)scr->pixels + j*scr->pitch/2 + i) = 
				  SDL_MapRGB(scr->format, (Uint8)r, (Uint8)g, (Uint8)b); 

			}

		/*sdl stuff - unlocking screen/drawing. */
		if(SDL_MUSTLOCK(scr)) SDL_UnlockSurface(scr); 
		/* if(sev.type == SDL_KEYDOWN) SDL_Quit(); */
		SDL_Delay(10); 
		SDL_Flip(scr); 
		/*or SDL_UpdateRect(screen, 0, 0, width, height);*/

	}

	fprintf(stderr, "end of input data. \n"); 

	SDL_Quit(); 

	/* finishing the process. */
	etheora_dec_finish(&ec); 


	return 0; 
}


