
this example features 2 programs: a server that encodes some data and accepts
a connection from clients and a client that connects to that server, get 
data and draw it into an SDL window. 


building example:
copy (or link) etheora.c etheora.h etheora-int.h to this directory. 
build the client with (requires libSDL in your system, with 
development files, as well libtheora): 

gcc client-decoder.c etheora.c  -I. -ltheora -lSDL -o client-decoder

and then build the server with: 
gcc server-encoder.c etheora.c  -I. -ltheora  -o server-encoder

Running example: 
./server-encoder 3000
./client-decoder localhost 3000

