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


int cleanServers2(char *error)
{
	char	sql[8096];
	char	sid[2046];
	int	i;
	MYSQL_ROW	row;
	MYSQL_ROW	row2;
	int	existing = 0;
	int	nrows = 0;
	int	nrows2 = 0;

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"SELECT id FROM servers");
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
				sprintf(sql,"SELECT count(*) FROM server_details where parent_id = %s", row[0]);
				if(mysql_real_query(&dbase,sql,strlen(sql))) {
					strcpy(error, mysql_error(&dbase));
					return(YP_ERROR);
				}
				result2 = mysql_store_result(&dbase);
				nrows2 = mysql_num_rows(result2);
				if(nrows2 != 0) {
					row2 = mysql_fetch_row(result2);
					if (row2[0]) {
						if (atoi(row2[0]) == 0) {
							sprintf(sql,"delete from servers where id = '%s'", row[0]);
							if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
								mysql_free_result(result);
								mysql_free_result(result2);
								return(YP_ERROR);
							}
							LogMessage(LOG_INFO, "ID (%s) removed from servers due to something being whacked", row[0]);
						}
					}
				}
				mysql_free_result(result2);
			}
		}
		mysql_free_result(result);
		sprintf(sql,"COMMIT");
		mysql_real_query(&dbase,sql,strlen(sql));
	}
	return(YP_SUCCESS);

}
int cleanServers3(char *error)
{
	char	sql[8096];
	char	sid[2046];
	int	i;
	MYSQL_ROW	row;
	MYSQL_ROW	row2;
	int	existing = 0;
	int	nrows = 0;
	int	nrows2 = 0;
        char    detail_id[255] = "";
        char    parent_id[255] = "";
	char	*p1 = NULL;

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"SELECT id FROM servers_touch");
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
				memset(parent_id, '\000', sizeof(parent_id));
				memset(detail_id, '\000', sizeof(detail_id));
				p1 = strchr(row[0], '-');
				if (p1) {
					strncpy(parent_id, row[0], p1-row[0]);
					strcpy(detail_id, p1+1);
				}
				sprintf(sql,"SELECT count(*) FROM servers where id = %s", parent_id);
				if(mysql_real_query(&dbase,sql,strlen(sql))) {
					strcpy(error, mysql_error(&dbase));
					return(YP_ERROR);
				}
				result2 = mysql_store_result(&dbase);
				nrows2 = mysql_num_rows(result2);
				if(nrows2 != 0) {
					row2 = mysql_fetch_row(result2);
					if (row2[0]) {
						if (atoi(row2[0]) == 0) {
							sprintf(sql,"delete from servers_touch where id = '%s'", row[0]);
							if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
								mysql_free_result(result);
								mysql_free_result(result2);
								return(YP_ERROR);
							}
							sprintf(sql,"delete from server_details where parent_id = %s", parent_id);
							if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
								mysql_free_result(result);
								mysql_free_result(result2);
								return(YP_ERROR);
							}
							LogMessage(LOG_INFO, "ID (%s) removed from servers due to something being whacked", row[0]);
						}
					}
				}
				mysql_free_result(result2);
			}
		}
		mysql_free_result(result);
		sprintf(sql,"COMMIT");
		mysql_real_query(&dbase,sql,strlen(sql));
	}
	return(YP_SUCCESS);

}
int cleanServers4(char *error)
{
	char	sql[8096];
	char	sid[2046];
	int	i;
	MYSQL_ROW	row;
	MYSQL_ROW	row2;
	int	existing = 0;
	int	nrows = 0;
	int	nrows2 = 0;
        char    detail_id[255] = "";
        char    parent_id[255] = "";
        char    id[255] = "";
	char	*p1 = NULL;

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"SELECT id, parent_id FROM server_details");
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
				sprintf(id, "%s-%s", row[1], row[0]);
				sprintf(sql,"SELECT count(*) FROM servers_touch where id = '%s'", id);
				if(mysql_real_query(&dbase,sql,strlen(sql))) {
					strcpy(error, mysql_error(&dbase));
					return(YP_ERROR);
				}
				result2 = mysql_store_result(&dbase);
				nrows2 = mysql_num_rows(result2);
				if(nrows2 != 0) {
					row2 = mysql_fetch_row(result2);
					if (row2[0]) {
						if (atoi(row2[0]) == 0) {
							sprintf(sql,"delete from server_details where id = '%s'", row[0]);
							if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
								mysql_free_result(result);
								mysql_free_result(result2);
								return(YP_ERROR);
							}
							LogMessage(LOG_INFO, "Server Details (%s)(%s) because it didn't have a record (%s) in servers_touch", row[0], row[1], id);
						}
					}
				}
				mysql_free_result(result2);
				sprintf(sql,"SELECT count(*) FROM servers where id = %s", row[1]);
				if(mysql_real_query(&dbase,sql,strlen(sql))) {
					strcpy(error, mysql_error(&dbase));
					return(YP_ERROR);
				}
				result2 = mysql_store_result(&dbase);
				nrows2 = mysql_num_rows(result2);
				if(nrows2 != 0) {
					row2 = mysql_fetch_row(result2);
					if (row2[0]) {
						if (atoi(row2[0]) == 0) {
							sprintf(sql,"delete from server_details where id = '%s'", row[0]);
							if (mysql_real_query(&dbase,sql,strlen(sql))) {
								sprintf(error, "servers: %s", mysql_error(&dbase));
								sprintf(sql,"ROLLBACK");
								mysql_real_query(&dbase,sql,strlen(sql));
								mysql_free_result(result);
								mysql_free_result(result2);
								return(YP_ERROR);
							}
							LogMessage(LOG_INFO, "ID (%s) removed from server_details due to something being whacked", row[0]);
						}
					}
				}
				mysql_free_result(result2);
			}
		}
		mysql_free_result(result);
		sprintf(sql,"COMMIT");
		mysql_real_query(&dbase,sql,strlen(sql));
	}
	return(YP_SUCCESS);

}

int cleanServers(char *error)
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


	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"select id from servers_touch where last_touch < NOW() - INTERVAL 5 MINUTE");
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
				memset(parent_id, '\000', sizeof(parent_id));
				memset(detail_id, '\000', sizeof(detail_id));
				p1 = strchr(row[0], '-');
				if (p1) {
					strncpy(parent_id, row[0], p1-row[0]);
					strcpy(detail_id, p1+1);
				}
			
				sprintf(sql,"delete from server_details where id = '%s'", detail_id);
				if (mysql_real_query(&dbase,sql,strlen(sql))) {
					sprintf(error, "servers: %s", mysql_error(&dbase));
					sprintf(sql,"ROLLBACK");
					mysql_real_query(&dbase,sql,strlen(sql));
					mysql_free_result(result);
					return(YP_ERROR);
				}
				Log(LOG_DEBUG, "Check to see if there are any server details for parentid %s", parent_id);
				if (!anyMoreServerDetails(sid, error, parent_id)) {
					Log(LOG_DEBUG, "Nope, so lets delete the parent...");
					// If we are deleting the parent cluster, then whack all other clustered servers
					// this is a bit crappy, but the only way to preserve the RI...
					// Now go through and delete all server_touch records for the records we are
					// about to whack in server_details

					// Now delete the parent
					sprintf(sql,"delete from servers where id = %s", parent_id);
					if (mysql_real_query(&dbase,sql,strlen(sql))) {
						sprintf(error, "servers: %s", mysql_error(&dbase));
						sprintf(sql,"ROLLBACK");
						mysql_real_query(&dbase,sql,strlen(sql));
						return(YP_ERROR);
					}
				}
				else {
					Log(LOG_DEBUG, "Yep, so lets NOT delete the parent...");
				}

				sprintf(sql,"delete from servers_touch where id = '%s'", row[0]);
				if (mysql_real_query(&dbase,sql,strlen(sql))) {
					sprintf(error, "servers: %s", mysql_error(&dbase));
					sprintf(sql,"ROLLBACK");
					mysql_real_query(&dbase,sql,strlen(sql));
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
	setLogFile(YP_LOGDIR"yp-clean.log");

	if (connectToDB()) {
		ret = cleanServers(error);
		if (ret == YP_ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
		LogMessage(LOG_INFO, "Ran clean servers OK");
		ret = cleanServers2(error);
		if (ret == YP_ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
		LogMessage(LOG_INFO, "Ran clean servers 2 OK");
		ret = cleanServers3(error);
		if (ret == YP_ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
		LogMessage(LOG_INFO, "Ran clean servers 3 OK");
		ret = cleanServers4(error);
		if (ret == YP_ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
		LogMessage(LOG_INFO, "Ran clean servers 4 OK");
	}
	return(0);
}


