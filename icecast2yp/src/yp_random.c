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


int randomizeServers(char *error)
{
	char	sql[8096];
	char	sid[2046];
	int	i;
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
        char    detail_id[255] = "";
        char    parent_id[255] = "";
	char	*p1;
	int	randomNumber = 0;

	srand(time() + getpid());

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"select id from servers");
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
			row = mysql_fetch_row(result);
			if (row[0]) {
				randomNumber = rand();
				sprintf(sql,"update servers set rank = %d where id = %s", randomNumber, row[0]);
				if (mysql_real_query(&dbase,sql,strlen(sql))) {
					sprintf(error, "servers: %s", mysql_error(&dbase));
					sprintf(sql,"ROLLBACK");
					mysql_real_query(&dbase,sql,strlen(sql));
					mysql_free_result(result);
					return(YP_ERROR);
				}
			}
		}
		mysql_free_result(result);
		sprintf(sql,"COMMIT");
		mysql_real_query(&dbase,sql,strlen(sql));
	}
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
	setLogFile(YP_LOGDIR"yp-random.log");

	if (connectToDB()) {
		ret = randomizeServers(error);
		if (ret == YP_ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
	}
	return(0);
}


