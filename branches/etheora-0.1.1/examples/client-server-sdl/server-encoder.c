/*Copyright, 2007, by Ribamar Santarosa, ribamar@gmail.com */

/*a very simple theora video encoder using etheora api.
  play with etheora_enc_rgb_draw() r, g, b parameters and get
  different videos. 
  this example is a server which accepts a connection from
  clients, and sends it ogg data. 
  
  usage: ./server-encoder [port] 
  port - a tcp port where to listen incomming connections. it not
  provided, 4443 is used. 

  build command line example (with gcc): 
  gcc server-encoder.c path/to/etheora.c  -Ipath/to/etheora.h/dir \
  -ltheora   -o server-encoder
*/

#include <etheora.h>
#include <stdio.h>
#include <string.h>

/*tcp includes.*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>


#define NUMBER_OF_FRAMES 200 /* video with 200 frames.*/
#define PORT 4443  /* tcp port where to listen. */

int tcp_listen(FILE *finfo, int port) {
        int sock;
        struct sockaddr_in sin;
        int v = 1;

	/* getting a socket. */
        if((sock = socket(AF_INET,SOCK_STREAM, 0)) < 0){
                fprintf(finfo, "Can't create socket.\n");
                return -1;
        }

	/* binding the socket to a port. */
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &v,
                        sizeof(v));

        if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0){
                fprintf(finfo, "Can't bind.\n");
                return -2;
        }

	/* waiting for a conection. */
        if(listen(sock, 1024) < 0){
                fprintf(finfo, "Can't listen.\n");
                return -3;
	}

        return(sock);
}



int main(int argc, char **args){
	int f, i, j, sock, fout_fd, port; 
	etheora_ctx ec; 
	char *vendor = "etheora/libtheora"; 
	FILE *finfo, *fout; 

	/* opening video output file and file to print debuf info. as 
	   always, sockets could be used.Degub.txt can have a fast 
	   increase, you may want open a null device file as 
	   /dev/null. */

	/* listening. */
	(argc > 1) ? (port = atoi(args[1])) : (port = PORT); 
	if( (sock = tcp_listen(stderr, port)) < 0 ){
		fprintf(stderr, "usage: \n%s [PORT]\n", args[0]);
		return 1; 
	}

while(1){ /* forked while */
	if( (fout_fd = accept(sock, 0, 0) ) < 0 ){
		fprintf(stderr, "Can't accept connection.\n");
		return 1;
	}
	if(fork()) continue; 

	finfo = fopen("Debug.txt", "w"); 
	fprintf(stderr, "debug info in Debug.txt.\n"); 

	fout = fdopen(fout_fd, "w");
	if(fout == NULL || finfo == NULL){
		fprintf(stderr, "Debug.txt or output file"
			" couldn't be open.\n");
		return 1;
	}


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
					/*g*/ (3*i*i + j +f) % 256, 
					/*b*/ (j*i-f) % 256);

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

	fprintf(stderr, "video generated. \n"); 
	return 0; 
} /* forked while */

}


