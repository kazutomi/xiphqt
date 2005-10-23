#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<stdio.h>
#include	<time.h>
#include	<signal.h>
#include	<string.h>


int connect_w_to(char *ip, int port) { 
  int res; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 
  int soc = 0;
  struct hostent *server;
  char buff[1024] = "";


  // Create socket 
  soc = socket(AF_INET, SOCK_STREAM, 0); 
  if (soc < 0) { 
     fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno)); 
     exit(0); 
  } 

  addr.sin_family = AF_INET; 
  addr.sin_port = htons(port); 

  server = gethostbyname(ip);
  if (server == NULL) {
      //fprintf(stderr,"ERROR, no such host (%s)\n", ip);
      return(0);
  }
  strcpy(buff, (char *)inet_ntoa(*(struct in_addr *)server->h_addr));

  addr.sin_addr.s_addr = inet_addr(buff); 

  //bzero((char *) &addr, sizeof(addr));
  //bcopy((char *)server->h_addr, (char *)&addr.sin_addr.s_addr, server->h_length);

  // Set non-blocking 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     return(0);
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     return(0);
  } 
  // Trying to connect with timeout 
  res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        do { 
           tv.tv_sec = 10; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(soc, &myset); 
           res = select(soc+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              //fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
		 close(soc);
		return(0);
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
		 close(soc);
                 return(0);
              } 
              // Check the value returned... 
              if (valopt) { 
                 //fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt));
		 close(soc);
                 return(0);
              } 
              break; 
           } 
           else { 
              //fprintf(stderr, "Timeout!\n");
		 close(soc);
              return(0);
           } 
        } while (1); 
     } 
     else { 
        //fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        return(0);
     } 
  } 
 close(soc);
  return(1);
} 
