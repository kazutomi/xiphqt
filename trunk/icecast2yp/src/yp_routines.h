#ifndef __YP_ROUTINES_H__
#define __YP_ROUTINES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include "cgi.h"
#include <unistd.h>
#include "log.h"

#define NO_ACTION	0
#define ADD	1
#define TOUCH	2
#define REMOVE	3

#define YP_ERROR -1
#define YP_EXISTS 1
#define YP_ADDED 2
#define YP_TOUCHED 3
#define YP_REMOVED 4
#define YP_SUCCESS 5

#define AUDIOCAST_RESPONSE 1
#define ICECAST2_RESPONSE 2

#define AUDIOCAST_TOUCH 1
#define NORMAL_TOUCH 2


double  GetCurrentTime();
int genSID(char *sid, char *error);
int connectToDB() ;
int addServer(char *server_name, char *genre, char *cluster_password, char *desc, char *url, char *listenurl, char *server_type, char *bitrate, char *listing_ip, char *sid, char *samplerate, char *channels, char *error);
int touchServer(char *sid, char *touchip, char *cluster_password, char *song, char *listeners, char *genre, char *desc, char *listenurl, char *server_type, char *bitrate, char *server_name, char *error, int touchType);
int removeServer(char *sid, char *removeip, char *cluster_password, char *error);
void sendOK();
void sendHTML(char *html);
void sendYPResponse(int errorcode, char *msg, int type);
void disconnectFromDB();
char *url_decode(const char *src);
#endif
