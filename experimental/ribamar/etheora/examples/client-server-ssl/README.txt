
this example features 2 programs: a server that encodes some data and accepts
connections from clients. 




command line example - client: 
 gcc client-decoder.c etheora.c  -I. -ltheora -lSDL -o client-decoder
 gcc client-decoder.c etheora.c  -I. -ltheora -lssl -lSDL -o client-decoder

command line example - server: 
 gcc server-encoder.c etheora.c  -I. -ltheora  -o server-encoder

gcc fe.c -lssl -o fe
