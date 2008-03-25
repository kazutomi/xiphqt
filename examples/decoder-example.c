/*Copyright, 2007, by Ribamar Santarosa, ribamar@gmail.com */

/*a very simple theora video decoder using etheora api. 
  nothing is done with the pixels read from the frame buffers. 
  so you can choose what to do with them. 
  
  usage: 
  ./decoder-example [file.ogg] 
  file.ogg: file to read video input. if none is passed, data is read
  from standard input.

  build command line example (with gcc): 
  gcc decoder-example.c etheora.c  -I. -ltheora -o decoder-example

*/

#include <etheora.h>
#include <stdio.h>


int main(int argc, char **args){
	int i, j; 
	etheora_ctx ec; 
	FILE *finfo, *fin; 
	float r, g, b; 

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
		fprintf(stderr, "Can't start.\n"); 
		return 2; 
	}

	/*getting next frame from decoder.*/
	while(!etheora_dec_nextframe(&ec)){

		/*now we can read the frame buffer data. */
		for( i = 0; i < etheora_get_width(&ec); i++)
			for( j = 0; j < etheora_get_height(&ec); j++){
				/* or use etheora_dec_yuv draw() to 
			           read in yuv colorspace.*/
				etheora_dec_rgb_read(&ec, i, j, 
					 &r, &g, &b);  

				/* HERE YOU MAY DO WHAT YOU WANT
				  WITH THE PIXEL. */

			}

	}

	fprintf(stderr, "input end of data. \n"); 

	/* finishing the process. */
	etheora_dec_finish(&ec); 


	return 0; 
}


