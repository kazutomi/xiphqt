/*Copyright, 2007, by Ribamar Santarosa, ribamar@gmail.com */

/*
  a simple theora video decoder using etheora. this 
  example is an client which reads data from a (encoder) server. data
  is read from the server though a socket. Video is outputted 
  into an SDL window. 

  note this doesn't have a good performance yet and can't deal with 
  the real fps rate. Its performance is improved by defining OVERLAY, 
  however, as of this writing (19-nov-2007) etheora doesn't provide 
  yet a good way to use overlays. 

  usage: 
  ./client-decoder server_hostname server_port

  build command line example (with gcc): 
  gcc client-decoder.c path/to/etheora.c  -I/path/to/etheora.h/dir \
  		-ltheora -lSDL -o client-decoder

*/

#include <etheora.h>
#include <stdio.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_endian.h>

/*tcp includes*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>


#define YUV_DECODING 1
#define OVERLAY 1

int tcp_socket_connect(char *host, int port, FILE *finfo){
        struct hostent *h;
        int sock;
        struct sockaddr_in addr;

	/* getting the host address. */
        if(! (h = gethostbyname(host)) ){
                fprintf(finfo, "Couldn't resolve host.\n");
                return -1;
        }

	/* getting a socket. */
        if((sock = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP)) < 0){
                fprintf(finfo, "Couldn't create socket.\n");
                return -2;
        }

	/* connecting to the address. */
        memset(&addr, '\0', sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr = *(struct in_addr*) h->h_addr_list[0];
        addr.sin_port = htons(port);

        if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
                fprintf(finfo, "Couldn't connect socket.\n");
                return -3;
        }

        return sock;
}





/*taken from theora player_example.c, but not cropping yet. */

static void video_write(SDL_Surface *screen, SDL_Overlay *yuv_overlay, 
	yuv_buffer *yuv, SDL_Rect rect){
	int i;
	/*int crop_offset;*/

	/* Lock SDL_yuv_overlay */
	if ( SDL_MUSTLOCK(screen) ) {
		if ( SDL_LockSurface(screen) < 0 ) return;
	}
	if (SDL_LockYUVOverlay(yuv_overlay) < 0) return;

	/* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
	/* deal with border stride */
	/* reverse u and v for SDL */
	/* and crop input properly, respecting the encoded frame rect */
	/* crop_offset=ti.offset_x+yuv.y_stride*ti.offset_y; */
	for(i=0;i<yuv_overlay->h;i++)
		/* copy y*/
		memcpy(yuv_overlay->pixels[0]+yuv_overlay->pitches[0]*i,
				yuv->y /*+crop_offset*/ +yuv->y_stride*i,
				yuv_overlay->w);
	/* crop_offset=(ti.offset_x/2)+(yuv.uv_stride)*(ti.offset_y/2); */
	for(i=0;i<yuv_overlay->h/2;i++){
		/* copy v */
		memcpy(yuv_overlay->pixels[1]+yuv_overlay->pitches[1]*i,
				yuv->v+ /* crop_offset */ +yuv->uv_stride*i,
				yuv_overlay->w/2);
		/* copy u */
		memcpy(yuv_overlay->pixels[2]+yuv_overlay->pitches[2]*i,
				yuv->u+/* crop_offset+ */yuv->uv_stride*i,
				yuv_overlay->w/2);
	}

	/* Unlock SDL_yuv_overlay */
	if ( SDL_MUSTLOCK(screen) ) {
		SDL_UnlockSurface(screen);
	}
	SDL_UnlockYUVOverlay(yuv_overlay);


	/* Show, baby, show! */
	SDL_DisplayYUVOverlay(yuv_overlay, &rect);

}


int main(int argc, char **args){
	int i, j, sock; 
	etheora_ctx ec; 
	FILE *finfo, *fin; 
#ifdef YUV_DECODING
	unsigned char y, u, v; 
#endif
	float r, g, b;
	SDL_Surface *scr; 
	SDL_Event sev; 
#ifdef OVERLAY
	SDL_Rect rect; 
	SDL_Overlay *yuv_overlay;
#endif

	/* opening video input file and file to print debuf info. as 
	   always, sockets could be used. Degub.txt can have a fast 
	   increase, you may want open a null device file as 
	   /dev/null. */
	finfo = fopen("Debug.txt", "w"); 
	fprintf(stderr, "debug info in Debug.txt.\n"); 

	if(argc > 2){
		if( (sock = tcp_socket_connect(args[1], 
					atoi(args[2]), stderr)) < 0){
			fprintf(stderr, "Can't create socket.\n"); 
			return 1; 
		}
	}
	else{
		fprintf(stderr, "usage: \n%s host port\n", args[0]); 
		return 1; 
	}

	/* getting the FILE * from the socket. */
	fin = fdopen(sock, "r"); 
	if(fin == NULL || finfo == NULL){
		fprintf(stderr, "Debug.txt or input file couldn't be open.\n"); 
		return 1; 
	}

	/* configuring decoder. */
	etheora_dec_setup(&ec, fin, finfo);

	/* start the decoder. */
	if (etheora_dec_start(&ec)){
		fprintf(stderr, "Can't start decoding.\n"); 
		return 2; 
	}

	/*sdl stuff - setting video operation. */
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		fprintf(stderr, "Can't start SDL: %s.\n",  SDL_GetError()); 
		return 3; 
	}
#ifdef OVERLAY
	scr = SDL_SetVideoMode(etheora_get_width(&ec), etheora_get_height(&ec),
		/*0, SDL_SWSURFACE | SDL_ANYFORMAT);*/
		16, SDL_SWSURFACE); /*TODO: isn't this plat. dependent? */
#else 
	scr = SDL_SetVideoMode(etheora_get_width(&ec), etheora_get_height(&ec),
		0, SDL_SWSURFACE); 
#endif
	if (scr == NULL){
		fprintf(stderr, "Can't set video: %s.\n",  SDL_GetError()); 
		return 4; 
	}
	fprintf(stderr, "Bytes per pixel: %i.\n", scr->format->BytesPerPixel); 

#ifdef OVERLAY
	yuv_overlay = SDL_CreateYUVOverlay(etheora_get_width(&ec), 
			etheora_get_height(&ec),
			SDL_YV12_OVERLAY, scr);
	if ( yuv_overlay == NULL ) {
		fprintf(stderr, "SDL: Couldn't create SDL_yuv_overlay: %s\n",
				SDL_GetError());
		return 5; 
	}
	if ( yuv_overlay->hw_overlay == 1) {
		fprintf(stderr, "Using hardware acceleration.\n"); 
	}
	else{
		fprintf(stderr, "NOT using Hardware acceleration.\n"); 
	}

	rect.x = 0;
	rect.y = 0;
	rect.w = etheora_get_width(&ec); 
	rect.h = etheora_get_height(&ec); 

	SDL_DisplayYUVOverlay(yuv_overlay, &rect);
#endif

	SDL_WaitEvent(&sev); 
	/*getting next frame from decoder or a window quit act.*/
	while( !etheora_dec_nextframe(&ec) && sev.type != SDL_QUIT ){

		/*sdl stuff - locking the screen so we can draw in the sdl
		 * memory without changes appearing in  the screen. */
#ifndef OVERLAY
		if(SDL_MUSTLOCK(scr)) SDL_LockSurface(scr); 
#endif /* TODO: not original*/

		/*now we can read the frame buffer data. */
		for( i = 0; i < etheora_get_width(&ec);  i++)
			for( j = 0; j < etheora_get_height(&ec); j++){
#ifdef YUV_DECODING
				etheora_dec_yuv_read(&ec, i, j, 
					 &y, &u, &v);  
				etheora_pixel_yuv2rgb(y, u, v, &r, &g, &b);
				etheora_pixel_rgb_reescale(&r, &g, &b);
				etheora_pixel_rgb_unreescale(&r, &g, &b);
				etheora_enc_rgb_draw(&ec, i, j, r, g, b); 
				//etheora_enc_yuv_draw(&ec, i, j, y, u, v); 

#else
				/* or use etheora_dec_yuv draw() to 
			           read in yuv colorspace.*/
				etheora_dec_rgb_read(&ec, i, j, 
					 &r, &g, &b);  
#endif 

				  /* sdl - drawing the pixel. */
#ifdef YUV_DECODING
				  Uint32 a = 0; 
				  a = (Uint32) 0
				  	  + (((Uint32)y)*1 << 3*8)
				  	  + (((Uint32)r)*1 << 2*8) 
					  + (((Uint32)g)*1 << 1*8) 
					  + (((Uint32)b)*1 << 0*8); 

				  *((Uint32 *)scr->pixels + (j*scr->pitch)/4 + i)
				  	= a; 
				  

#else
				  *((Uint32 *)scr->pixels + j*scr->pitch/4 + i)=
					  SDL_MapRGB(scr->format, (Uint8)r, 
					  		(Uint8)g, (Uint8)b); 

#endif 
			}

#ifndef OVERLAY /* TODO: not original*/
		/* sdl stuff - unlocking screen/drawing. */
		if(SDL_MUSTLOCK(scr)) SDL_UnlockSurface(scr); 
#else 
		video_write(scr, yuv_overlay, &ec.yuv,  rect);

#endif 
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

