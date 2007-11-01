/*Copyright, 2007, by Ribamar Santarosa, ribamar@gmail.com */

/*a very simple theora video decoder using etheora api. 
  the frames are drawn into an opencv window. requires 
  opencv/highgui. 

  this simple program ignores the video frame per second 
  rate and works in less than 10 fps always.  also only 
  video is treated. 
  
  usage: 
  ./decoder-example-opencv [file.ogg] 
  file.ogg: file to read video input. if none is passed, 
  data is read from standard input.

  build command line example (with gcc):
  gcc -Wall decoder-example-opencv.c etheora.c  \
  -lhighgui -lstdc++  -I. -ltheora -o decoder-example-opencv
*/

#include <etheora.h>
#include <stdio.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>



int main(int argc, char **args){
	int i, j; 
	etheora_ctx ec; 
	FILE *finfo, *fin; 
	float r, g, b; 
	IplImage *image;

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

	/* opencv stuff - initiating a image and create an window.*/
	image = cvCreateImage (cvSize(ec.ti.frame_width, ec.ti.frame_height),
			IPL_DEPTH_8U, 3);
	cvvNamedWindow("BGR", 1);

	/*getting next frame from decoder.*/
	while(!etheora_dec_nextframe(&ec)){

		/*now we can read the frame buffer data. */
		for( i = 0; i < etheora_get_width(&ec); i++)
			for( j = 0; j < etheora_get_height(&ec); j++){
				/* or use etheora_dec_yuv draw() to 
			           read in yuv colorspace.*/
				etheora_dec_rgb_read(&ec, i, j, 
					 &r, &g, &b);  

				/* opencv stuff - drawing the pixel in the
				   opencv image. */
				CV_IMAGE_ELEM( image, uchar, j,
						1*(i*3) + 0 ) = (uchar) b;
				CV_IMAGE_ELEM( image, uchar, j,
						1*(i*3) + 1 ) = (uchar) g;
				CV_IMAGE_ELEM( image, uchar, j,
						1*(i*3) + 2 ) = (uchar) r;
			}

		/* opencv stuff - drawing image in the 
		   window and wait 100 ms (this simple
		   program doesn't care about the frame 
		   per second rate).*/
		cvShowImage("BGR", image);
		cvWaitKey(10);


	}

	fprintf(stderr, "input end of data. \n"); 

	/* opencv stuff - showing last frame and waiting a key.*/
	cvShowImage("BGR", image);
	cvWaitKey( 0 );
	cvDestroyWindow("BGR");

	/* finishing the process. */
	etheora_dec_finish(&ec); 


	return 0; 
}


