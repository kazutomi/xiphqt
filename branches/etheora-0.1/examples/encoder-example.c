/*Copyright, 2007, by Ribamar Santarosa, ribamar@gmail.com */

/*a very simple theora video encoder using etheora api.
  play with etheora_enc_rgb_draw() r, g, b parameters and get
  different videos. 
  
  usage: ./encoder-example [file.ogg] 
  file.ogg: file to be outputed. if none is passed, video data 
  is printed to standard output. 

  build command line example (with gcc): 
  gcc encoder-example.c etheora.c  -I. -ltheora -o encoder-example
*/

#include <etheora.h>
#include <stdio.h>

#define NUMBER_OF_FRAMES 20 /* video with 20 frames.*/

int main(int argc, char **args){
	int f, i, j; 
	etheora_ctx ec; 
	char *vendor = "etheora/libtheora"; 
	FILE *finfo, *fout; 

	/* opening video output file and file to print debuf info. as 
	   always, sockets could be used.Degub.txt can have a fast 
	   increase, you may want open a null device file as 
	   /dev/null. */

	finfo = fopen("Debug.txt", "w"); 
	if(argc > 1) {
		fout = fopen(args[1], "w");
		fprintf(stderr, "opening %s to write video file . \n", args[1]);
	}
	else {  
		fout = stdout;
		fprintf(stderr, "opening standard output to write video file. \n");
	}
	if(fout == NULL || finfo == NULL){
		fprintf(stderr, "Debug.txt or output file couldn't be open.\n");
		return 1;
	}


	fprintf(stderr, "debug info in Debug.txt.\n"); 

	/* configuring encoder: 640x480 video 12/1 frames per second. */
	etheora_enc_setup(&ec, 640, 480, ETHEORA_ASPECT_NORMAL, 
				12, 1, fout, finfo);

	/* last chance to change theora parameters before encoding.
	   the experienced user can change ec.ti parameters or edit
	   the ec.tc structure. */
	ec.ti.target_bitrate = 200000; /*200 kbps*/
	ec.tc.vendor = vendor; 

	/* start the encoder. */
	if (etheora_enc_start(&ec)){
		fprintf(stderr, "Can't start.\n"); 
		return 2; 
	}

	for ( f = 0; f < NUMBER_OF_FRAMES - 1; f++){

		/*now we're ready to draw the frames. */
		for( i = 0; i < 640; i++)
			for( j = 0; j < 480; j++)
			/* or use etheora_enc_yuv_draw() to 
			   draw in yuv colorspace.*/
				etheora_enc_rgb_draw(&ec, i, j, 
					/*r*/ 255, 
					/*g*/ (i*i+i*j*j-f) % 256, 
					/*b*/ (j*i+i*i-f) % 256);

		/*submiting drawn frame to encoder.*/
		etheora_enc_nextframe(&ec); 
	}

	/*drawing last frame.*/
	for( i = 0; i < 640; i++)
		for( j = 0; j < 480; j++)
			etheora_enc_rgb_draw(&ec, i, j, 
					/*r*/ 255, 
					/*g*/ (i*i+i*j*j-f) % 256, 
					/*b*/ (j*i+i*i-f) % 256);

	/*submiting last frame to encoder and finishing the process. */
	etheora_enc_finish(&ec); 

	fprintf(stderr, "video file generated. \n"); 

	return 0; 
}


