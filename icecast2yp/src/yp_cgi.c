/*********************************************
YP-CGI by oddsock
**********************************************/
#include "yp_routines.h"


int main(int argc, char * argv[])
{
	int res;
	int	ret = 0;
	int	ok = 0;
	int	action = NO_ACTION;
	char	error[2046];
	char	listing_ip[255];
	char	server_name[101];
	char	server_type[26];
	char	server_subtype[255];
	char	bitrate[26];
	char	desc[256];
	char	st[256];
	char	genre[101];
	char	sid[101];
	char	cluster_password[51];
	char	url[256];
	char	current_song[256];
	char	listenurl[201];
	char	listenurl2[201];
	char	listeners[256];
	char	samplerate[256];
	char	channels[256];
	char	msg[1024];
	char *p1 = 0;
	int	use_listingIP = 0;


	setErrorType(LM_DEBUG);
	setLogFile(YP_LOGDIR"yp_cgi.log");
	memset(error, '\000', sizeof(error));
	memset(listing_ip, '\000', sizeof(listing_ip));
	memset(server_name, '\000', sizeof(server_name));
	memset(server_type, '\000', sizeof(server_type));
	memset(server_subtype, '\000', sizeof(server_subtype));
	memset(bitrate, '\000', sizeof(bitrate));
	memset(desc, '\000', sizeof(desc));
	memset(genre, '\000', sizeof(genre));
	memset(sid, '\000', sizeof(sid));
	memset(cluster_password, '\000', sizeof(cluster_password));
	memset(url, '\000', sizeof(url));
	memset(current_song, '\000', sizeof(current_song));
	memset(listenurl, '\000', sizeof(listenurl));
	memset(listenurl2, '\000', sizeof(listenurl2));
	memset(listeners, '\000', sizeof(listeners));
	memset(samplerate, '\000', sizeof(samplerate));
	memset(channels, '\000', sizeof(channels));
	memset(msg, '\000', sizeof(msg));
	memset(st, '\000', sizeof(st));

	Log(LOG_DEBUG, "Starting execution");
	if (getenv("REMOTE_ADDR")) {	
		strcpy(listing_ip, getenv("REMOTE_ADDR"));
	}
	setLogIP(listing_ip);
	cgi_init();
	cgi_process_form();

	//sendOK();

	/* Was there an error initializing the CGI??? */
/*

	if (res != CGIERR_NONE) {
		sprintf(msg, "Error # %d: %s<p>\n", res, cgi_strerror(res));
		sendYPResponse(0, msg, ICECAST2_RESPONSE);
	}
*/
	res = connectToDB();
	if (!res) {
		sendYPResponse(0, "Error connecting to database", ICECAST2_RESPONSE);
		goto endofcall;
	}

	Log(LOG_DEBUG, "Connected to DB");
	/* Grab some fields from an HTML form and display them: */

	if (cgi_param("action") != NULL) {
		if (!strncmp(cgi_param("action"), "add", strlen("add"))) {
			action = ADD;
			ok = 1;
		}
		if (!strncmp(cgi_param("action"), "touch", strlen("touch"))) {
			action = TOUCH;
			ok = 1;
		}
		if (!strncmp(cgi_param("action"), "remove", strlen("remove"))) {
			action = REMOVE;
			ok = 1;
		}
	}
	if (action == NO_ACTION) {
		sendYPResponse(0, "No action specified", ICECAST2_RESPONSE);
		ok = 0;
	}
	Log(LOG_DEBUG, "Action = %d - OK = %d", action, ok);
	if (ok) {
		char	*ptmp = NULL;
		if (cgi_param("sn") != NULL) {
			//printf("<li>Server Name : %s\n", cgi_param("sn"));
			Log(LOG_DEBUG, "Getting Server Name");
			ptmp = cgi_unescape_special_chars(cgi_param("sn"));
			if (ptmp) {
				strncpy(server_name, ptmp, sizeof(server_name)-1);
				free(ptmp);
			}
			
		}
		if (cgi_param("genre") != NULL) {
			Log(LOG_DEBUG, "Getting Genre");
			//printf("<li>Genre : %s\n", cgi_param("genre"));
			ptmp = cgi_unescape_special_chars(cgi_param("genre"));
			if (ptmp) {
				strncpy(genre, ptmp, sizeof(genre)-1);
				free(ptmp);
			}
		}
		if (cgi_param("cpswd") != NULL) {
			//printf("<li>Cluster Password : %s\n", cgi_param("cpswd"));
			Log(LOG_DEBUG, "Getting Cluster Password");
			ptmp = cgi_unescape_special_chars(cgi_param("cpswd"));
			if (ptmp) {
				strncpy(cluster_password, ptmp, sizeof(cluster_password)-1);
				free(ptmp);
			}
		}
		if (cgi_param("desc") != NULL) {
			//printf("<li>Decription : %s\n", cgi_param("desc"));
			Log(LOG_DEBUG, "Getting Desc");
			ptmp = cgi_unescape_special_chars(cgi_param("desc"));
			if (ptmp) {
				strncpy(desc, cgi_param("desc"), sizeof(desc)-1);
				free(ptmp);
			}
		}
		if (cgi_param("url") != NULL) {
			Log(LOG_DEBUG, "Getting URL");
			//printf("<li>URL : %s\n", cgi_param("url"));
			ptmp = cgi_unescape_special_chars(cgi_param("url"));
			if (ptmp) {
				strncpy(url, ptmp, sizeof(url)-1);
				free(ptmp);
			}
		}
		if (cgi_param("listenurl") != NULL) {
			//printf("<li>Listen URL : %s\n", cgi_param("listenurl"));
			Log(LOG_DEBUG, "Getting Listen URL");
			if (strlen(cgi_param("listenurl")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("listenurl"));
				if (ptmp) {
					strncpy(listenurl, ptmp, sizeof(listenurl)-1);
					if (!strncmp(listenurl, "http://localhost", strlen("http://localhost"))) {
						p1 = listenurl + strlen("http://localhost");
						use_listingIP = 1;
					}
					if (!strncmp(listenurl, "http://192.168.", strlen("http://192.168."))) {
						p1 = listenurl + strlen("http://192.168.");
						p1 = strchr(p1, ':');
						use_listingIP = 1;
					}
					if (use_listingIP) {
						if (p1) {
							snprintf(listenurl2, sizeof(listenurl2)-1, "http://%s%s", listing_ip, p1);
							strcpy(listenurl, listenurl2);
						}
					}
					free(ptmp);
				}
			}
		}
		if (cgi_param("type") != NULL) {
			Log(LOG_DEBUG, "Getting Server Type");
			if (strlen(cgi_param("type")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("type"));
				if (ptmp) {
					strncpy(server_type, ptmp, sizeof(server_type)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("stype") != NULL) {
			Log(LOG_DEBUG, "Getting Server SubType");
			if (strlen(cgi_param("stype")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("stype"));
				if (ptmp) {
					strncpy(server_subtype, ptmp, sizeof(server_subtype)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("b") != NULL) {
			Log(LOG_DEBUG, "Getting Bitrate");
			if (strlen(cgi_param("b")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("b"));
				if (ptmp) {
					strncpy(bitrate, ptmp, sizeof(bitrate)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("ice-bitrate") != NULL) {
			Log(LOG_DEBUG, "Getting Bitrate");
			if (strlen(cgi_param("ice-bitrate")) > 0) {
				memset(bitrate, '\000', sizeof(bitrate));
				ptmp = cgi_unescape_special_chars(cgi_param("ice-bitrate"));
				if (ptmp) {
					strncpy(bitrate, ptmp, sizeof(bitrate)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("ice-quality") != NULL) {
			Log(LOG_DEBUG, "Getting Quality");
			if (strlen(cgi_param("ice-quality")) > 0) {
				memset(bitrate, '\000', sizeof(bitrate));
				ptmp = cgi_unescape_special_chars(cgi_param("ice-quality"));
				if (ptmp) {
					snprintf(bitrate, sizeof(bitrate)-1, "Quality %s", ptmp);
					free(ptmp);
				}
			}
		}
		if (cgi_param("ice-samplerate") != NULL) {
			Log(LOG_DEBUG, "Getting Samplerate");
			if (strlen(cgi_param("ice-samplerate")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("ice-samplerate"));
				if (ptmp) {
					strncpy(samplerate, ptmp, sizeof(samplerate)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("ice-channels") != NULL) {
			Log(LOG_DEBUG, "Getting Channels");
			if (strlen(cgi_param("ice-channels")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("ice-channels"));
				if (ptmp) {
					strncpy(channels, ptmp, sizeof(channels)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("bitrate") != NULL) {
			Log(LOG_DEBUG, "Getting Bitrate");
			if (strlen(cgi_param("bitrate")) > 0) {
				memset(bitrate, '\000', sizeof(bitrate));
				ptmp = cgi_unescape_special_chars(cgi_param("bitrate"));
				if (ptmp) {
					strncpy(bitrate, ptmp, sizeof(bitrate)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("quality") != NULL) {
			Log(LOG_DEBUG, "Getting Quality");
			if (strlen(cgi_param("quality")) > 0) {
				memset(bitrate, '\000', sizeof(bitrate));
				Log(LOG_DEBUG, "Quality = (%s)", cgi_param("quality"));
				ptmp = cgi_unescape_special_chars(cgi_param("quality"));
				Log(LOG_DEBUG, "Quality = (%s)", ptmp);
				if (ptmp) {
					snprintf(bitrate, sizeof(bitrate)-1, "Quality %s", ptmp);
					free(ptmp);
				}
			}
		}
		if (cgi_param("samplerate") != NULL) {
			Log(LOG_DEBUG, "Getting Samplerate");
			if (strlen(cgi_param("samplerate")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("samplerate"));
				if (ptmp) {
					strncpy(samplerate, ptmp, sizeof(samplerate)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("channels") != NULL) {
			Log(LOG_DEBUG, "Getting Channels");
			if (strlen(cgi_param("channels")) > 0) {
				ptmp = cgi_unescape_special_chars(cgi_param("channels"));
				if (ptmp) {
					strncpy(channels, ptmp, sizeof(channels)-1);
					free(ptmp);
				}
			}
		}
		if (cgi_param("sid") != NULL) {
			Log(LOG_DEBUG, "Getting SID");
			ptmp = cgi_unescape_special_chars(cgi_param("sid"));
			if (ptmp) {
				strncpy(sid, ptmp, sizeof(sid)-1);
				free(ptmp);
			}
			setLogSid(sid);
		}
		if (cgi_param("st") != NULL) {
			Log(LOG_DEBUG, "Getting Song Title");
			ptmp = cgi_unescape_special_chars(cgi_param("st"));
			if (ptmp) {
				strncpy(st, ptmp, sizeof(st)-1);
				free(ptmp);
			}
		}
		if (cgi_param("listeners") != NULL) {
			Log(LOG_DEBUG, "Getting Listeners");
			ptmp = cgi_unescape_special_chars(cgi_param("listeners"));
			if (ptmp) {
				strncpy(listeners, ptmp, sizeof(listeners)-1);
				free(ptmp);
			}
		}
		Log(LOG_DEBUG, "Done getting parameters");
		if (action == ADD) {
			Log(LOG_DEBUG, "Checking server name");
			if (strlen(server_name) == 0) {
				sendYPResponse(0, "Server name blank", ICECAST2_RESPONSE);
				goto endofcall;
			}
			Log(LOG_DEBUG, "Checking Genre");
			if (strlen(genre) == 0) {
				sendYPResponse(0, "Genre blank", ICECAST2_RESPONSE);
				goto endofcall;
			}
			Log(LOG_DEBUG, "Checking Desc");
			if (strlen(desc) == 0) {
				strcpy(desc, server_name);
			}
			Log(LOG_DEBUG, "Checking listen URL");
			if (strlen(listenurl) == 0) {
				sendYPResponse(0, "Listen URL blank", ICECAST2_RESPONSE);
				goto endofcall;
			}
			Log(LOG_DEBUG, "Checking Server Type");
			if (strlen(server_type) == 0) {
				sendYPResponse(0, "Server Type blank", ICECAST2_RESPONSE);
				goto endofcall;
			}
			Log(LOG_DEBUG, "Checking Bitrate");
			if (strlen(bitrate) == 0) {
				strcpy(bitrate, "NA");
			}
			Log(LOG_DEBUG, "Going to Add Server");
			memset(sid, '\000', sizeof(sid));
			ret = addServer(server_name, genre, cluster_password, desc, url, listenurl, server_type, server_subtype, bitrate, listing_ip, sid, samplerate, channels, error);
			Log(LOG_DEBUG, "Done With Add Server");
			if (ret == YP_ADDED) {
				Log(LOG_DEBUG, "sending sid  of %s", sid);
				Log(LOG_INFO, "YP_ADD sid of %s", sid);
				printf("SID: %s\r\nTouchFreq: 60\r\n", sid);
				sendYPResponse(1, "Successfull Add", ICECAST2_RESPONSE);
				goto endofcall;
			}
			if (ret == YP_EXISTS) {
				Log(LOG_DEBUG, "Server Exists (%s)", server_name);
				Log(LOG_INFO, "YP_ADD failed (Server exists) %s", server_name);
				sendYPResponse(0, "Server already exists", ICECAST2_RESPONSE);
				goto endofcall;
			}
			if (ret == YP_ERROR) {
				Log(LOG_INFO, "YP_ADD failed error of %s", error);
				sendYPResponse(0, error, ICECAST2_RESPONSE);
				goto endofcall;
			}
		}
		if (action == TOUCH) {
			if (strlen(sid) == 0) {
				Log(LOG_INFO, "YP_TOUCH failed SID blank");
				sendYPResponse(0, "SID blank", ICECAST2_RESPONSE);
				goto endofcall;
			}
			ret = touchServer(sid, listing_ip, cluster_password, st, listeners, "", "", "", "", "", "", error, NORMAL_TOUCH);
			if (ret == YP_TOUCHED) {
				Log(LOG_INFO, "YP_TOUCH successfull sid (%s)", sid);
				sendYPResponse(1, "Successfull Touch", ICECAST2_RESPONSE);
				goto endofcall;
			}
			if (ret == YP_EXISTS) {
				Log(LOG_DEBUG, "Server Exists (%s)", server_name);
				Log(LOG_INFO, "YP_TOUCH failed server exists ?!?!? (%s)", server_name);
				sendYPResponse(0, "Server already exists", ICECAST2_RESPONSE);
				goto endofcall;
			}
			if (ret == YP_ERROR) {
				Log(LOG_INFO, "YP_TOUCH failed error of %s", error);
				sendYPResponse(0, error, ICECAST2_RESPONSE);
				goto endofcall;
			}
		}
		if (action == REMOVE) {
			if (strlen(sid) == 0) {
				Log(LOG_INFO, "YP_REMOVE failed SID blank");
				sendYPResponse(0, "SID blank", ICECAST2_RESPONSE);
				goto endofcall;
			}
			ret = removeServer(sid, listing_ip, cluster_password, error);
			if (ret == YP_TOUCHED) {
				Log(LOG_INFO, "YP_REMOVE successfull sid (%s)", sid);
				sendYPResponse(1, "Successfull Remove", ICECAST2_RESPONSE);
				goto endofcall;
			}
			if (ret == YP_ERROR) {
				Log(LOG_INFO, "YP_REMOVE failed error of %s", error);
				sendYPResponse(0, error, ICECAST2_RESPONSE);
				goto endofcall;
			}
		}
	}
	endofcall:

	cgi_end();

	Log(LOG_DEBUG, "Disconnecting from DB");
	disconnectFromDB();

	Log(LOG_DEBUG, "Ending execution");

	return(0);
}


