/*********************************************
YP-CLEAN by oddsock
**********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <unistd.h>
#include "log.h"


extern MYSQL		dbase;
MYSQL_RES	*result;
MYSQL_RES	*result2;

#define YP_ERROR 0
#define YP_SUCCESS 1


int checkServers(char *error)
{
	char	sql[8096];
	char	sid[2046];
	int	i;
	int	j;
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
        char    detail_id[255] = "";
        char    parent_id[255] = "";
	char	*p1;
	int	randomNumber = 0;
	int	goodCount = 0;
	int	badCount = 0;
	char	badSeen[2096][255];
	int	numBadSeen = 0;

	memset(&badSeen, '\000', sizeof(badSeen));
	srand(time() + getpid());

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"select a.id, listen_url, a.server_name, a.listing_ip from servers a, server_details b where a.id = b.parent_id and a.yp_status = 'notverified' order by listen_url");
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		return(YP_ERROR);
	}
	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	
	if(nrows == 0) {
		return(YP_SUCCESS);
	}
	else {
		for (i=0;i<nrows;i++) {
			//printf("%d servers left\n", nrows-i);
			row = mysql_fetch_row(result);
			char	sName[1024] = "";
			char	listingIP[255] = "";
			if (row[3]) {
				strncpy(listingIP, row[3], sizeof(listingIP)-1);
			}
			if (row[2]) {
				strncpy(sName, row[2], sizeof(sName)-1);
			}
			if (row[1]) {
				//printf("(%s)\n", row[1]);
				char *p1 = row[1] + strlen("http://");
				char *p2 = strchr(p1, '/');
				if (p2) {
					*p2 = '\000';
				}
				char host[1024] = "";
				int port = 0;
				char *p3 = strchr(p1, ':');
				if (p3) {
					memset(host, '\000', sizeof(host));
					if (p3-p1 < 1024) {
						char addr[255] = "";
						int seen = 0;
						int i = 0;

						strncpy(host, p1, p3-p1);
						port = atoi(p3+1);
						//printf("Checking host %s (%s) port (%d)\n", sName, host, port);

						snprintf(addr, 254, "%s:%d", host, port);

						for (i=0;i<numBadSeen;i++) {
							if (!strcmp(badSeen[i], addr)) {
								seen = 1;
								break;
							}
						}
						if (seen) {
							//printf("Seen this already...bad\n");
							badCount++;
						}
						else {
							if (connect_w_to(host, port)) {
							    //printf("%s is good\n", host);
							    sprintf(sql,"update servers set yp_status = 'verified' where id = %s", row[0]);
							    if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
							    }
							    goodCount++;
							}
							else {
							    LogMessage(LOG_INFO, "Failed Validation(%s): %s - %s", listingIP, sName, row[1]);
							    sprintf(sql,"delete from servers where id = %s", row[0]);
							    if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
							    }
								//printf("%s is bad\n", host);
								strcpy(badSeen[numBadSeen], addr);
								numBadSeen++;
								badCount++;
							}
						}
					}
				}
			}
		}
		mysql_free_result(result);
	}
	//printf("Good: %d\n", goodCount);
	//printf("Bad: %d\n", badCount);
	//for (j=0;j<numBadSeen;j++) {
		//printf("\tBAD: %s\n", badSeen[j]);
	//}
	return(YP_SUCCESS);
}

int main(int argc, char * argv[])
{
	int res;
	int	ret = 0;
	int	ok = 0;
	int	pid = 0;
	char	error[2046];
	int	loop = 1;

	memset(error, '\000', sizeof(error));

	setErrorType(LM_INFO);
	setLogFile(YP_LOGDIR"yp-check.log");

	if (connectToDB()) {
		ret = checkServers(error);
		if (ret == YP_ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
	}
	return(0);
}


