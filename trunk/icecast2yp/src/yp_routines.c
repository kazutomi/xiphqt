/*********************************************
YP-CGI by oddsock
**********************************************/

#include "yp_routines.h"
#include "yp_db.h"

static MYSQL_RES	*result;
MYSQL	dbase;

double  GetCurrentTime() {

        double  thetime;
 
        struct timeval  tv;
        gettimeofday(&tv, NULL);
        thetime = (double)tv.tv_sec + (double)((double)tv.tv_usec / (double)1000000);
        return thetime;
}
static int hex(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    else if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else
        return -1;
}

char *url_decode(const char *src)
{
    int len = strlen(src);
    unsigned char *decoded;
    int i;
    char *dst;
    int done = 0;
 
    decoded = calloc(1, len + 1);
 
    dst = decoded;
 
    for(i=0; i < len; i++) {
        switch(src[i]) {
            case '%':
                if(i+2 >= len) {
                    free(decoded);
                    return NULL;
                }
                if(hex(src[i+1]) == -1 || hex(src[i+2]) == -1 ) {
                    free(decoded);
                    return NULL;
                }
 
                *dst++ = hex(src[i+1]) * 16  + hex(src[i+2]);
                i+= 2;
                break;
            case '#':
                done = 1;
                break;
            case 0:
		Log(LOG_ERROR, "Major problem with url_decode()");
                free(decoded);
                return NULL;
                break;
            default:
                *dst++ = src[i];
                break;
        }
        if(done)
            break;
    }
 
    *dst = 0; /* null terminator */
 
    return decoded;
}

int genSID(char *sid, char *error)
{
	char	key[1024];
	int i = 0;
	char	sql[8096];
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;


	Log(LOG_DEBUG, "Generating SID");
	for (i=0;i<10;i++) {
		memset(key, '\000', sizeof(key));
		sprintf(key, "%f", GetCurrentTime());
		Log(LOG_DEBUG, "Key %d = %s", i, key);

		memset(sql, '\000', sizeof(sql));

		sprintf(sql,"select count(*) from servers where sid = \'%s\'",key);
		if(mysql_real_query(&dbase,sql,strlen(sql))) {
			strcpy(error, mysql_error(&dbase));
			return(YP_ERROR);
		}
		result = mysql_store_result(&dbase);
		nrows = mysql_num_rows(result);
		if(nrows == 0) {
			return(YP_ERROR);
		}
		else {
			row = mysql_fetch_row(result);
			mysql_free_result(result);
			existing = 0;
			if (row[0]) {
				existing = atoi(row[0]);
			}
			if (!existing) {
				strcpy(sid, key);
				break;
			}
		}
	}
	return(YP_SUCCESS);

}
void disconnectFromDB() {
	mysql_close(&dbase);
}
int connectToDB() {
	char	*pDBHost = NULL;
	char	*pDBUser = NULL;
	char	*pDBPassword = NULL;
	char	*pDBDatabase = NULL;
	FILE	*filep = NULL;
	char	envstring[255] = "";

	filep = fopen("/home/oddsock/yp.db", "r");
	if (filep) {
		while (!feof(filep)) {
			memset(envstring, '\000', sizeof(envstring));
			fgets(envstring, sizeof(envstring)-1, filep);	
			if (strlen(envstring) > 0) {
				putenv(envstring);
			}
		}
		fclose(filep);
	}


	if(mysql_init(&dbase) != NULL)	{
		if (mysql_real_connect(&dbase,YP_DB_HOST,YP_DB_USER,YP_DB_PASSWORD,YP_DB_DATABASE,3306,NULL,0) == NULL) {
			return 0;
		}
	}
	else {
		return 0;
	}
	return 1;
}
int checkForCluster(char *server_name, char *listing_ip, char *error, char *existing_id)
{
	char	sql[8096];
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
	char	*server_name_esc = NULL;

	memset(sql, '\000', sizeof(sql));

	server_name_esc = malloc(strlen(server_name)*2 + 1);
	memset(server_name_esc, '\000', strlen(server_name)*2 + 1);
	mysql_real_escape_string(&dbase, server_name_esc, server_name, strlen(server_name));

	snprintf(sql, sizeof(sql)-1, "select id from servers where server_name = \'%s\' and listing_ip = '%s'",server_name_esc, listing_ip);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		free(server_name_esc);
		return(YP_ERROR);
	}

	free(server_name_esc);
	server_name_esc = NULL;

	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	
	if(nrows == 0) {
		existing = 0;
	}
	else {
		row = mysql_fetch_row(result);
		mysql_free_result(result);
		if (row[0]) {
			existing = 1;
			strcpy(existing_id, row[0]);
		}
	}
	return existing;
}
int anyMoreServerDetails(char *sid, char *error, char *parent_id)
{
	char	sql[8096];
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;

	memset(sql, '\000', sizeof(sql));

	snprintf(sql, sizeof(sql)-1, "select count(*) from server_details where parent_id = %s",parent_id);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		return(YP_ERROR);
	}

	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	
	if(nrows == 0) {
		return(0);
	}
	else {
		row = mysql_fetch_row(result);
		mysql_free_result(result);
		if (row[0]) {
			if (atoi(row[0]) > 0) {
				return(1);
			}
		}
		else {
			return(0);
		}
	}
	return(0);
}
int getClusterIDBySID(char *sid, char *error, char *cluster_id)
{
	char	sql[8096];
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;

	memset(sql, '\000', sizeof(sql));

	snprintf(sql, sizeof(sql)-1, "select cluster_id from servers where id = %s",sid);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		return(YP_ERROR);
	}

	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	
	if(nrows == 0) {
		return(0);
	}
	else {
		row = mysql_fetch_row(result);
		mysql_free_result(result);
		if (row[0]) {
			strcpy(cluster_id, row[0]);
		}
		else {
			return(0);
		}
	}
	return(1);
}
int getClusterID(char *server_name, char *listing_ip, char *error, char *cluster_id)
{
	char	sql[8096];
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
	char	*server_name_esc = NULL;

	memset(sql, '\000', sizeof(sql));

	server_name_esc = malloc(strlen(server_name)*2 + 1);
	memset(server_name_esc, '\000', strlen(server_name)*2 + 1);
	mysql_real_escape_string(&dbase, server_name_esc, server_name, strlen(server_name));

	snprintf(sql, sizeof(sql)-1, "select cluster_id from servers where server_name = \'%s\' and listing_ip = '%s'",server_name_esc, listing_ip);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		free(server_name_esc);
		return(YP_ERROR);
	}

	free(server_name_esc);
	server_name_esc = NULL;

	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	
	if(nrows == 0) {
		return(0);
	}
	else {
		row = mysql_fetch_row(result);
		mysql_free_result(result);
		if (row[0]) {
			strcpy(cluster_id, row[0]);
		}
		else {
			return(0);
		}
	}
	return(1);
}
int updateClusterID(char *server_name, char *listing_ip, char *error, char *cluster_id)
{
	char	sql[8096];
	char	sql2[8096];
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
	char	*server_name_esc = NULL;

	memset(sql, '\000', sizeof(sql));
	memset(sql2, '\000', sizeof(sql2));

	server_name_esc = malloc(strlen(server_name)*2 + 1);
	memset(server_name_esc, '\000', strlen(server_name)*2 + 1);
	mysql_real_escape_string(&dbase, server_name_esc, server_name, strlen(server_name));

	snprintf(sql, sizeof(sql)-1, "update clustered_servers set cluster_id = %s where server_name = \'%s\' and listing_ip = '%s'",cluster_id, server_name_esc, listing_ip);
	snprintf(sql2, sizeof(sql2)-1, "update servers set cluster_id = %s where server_name = \'%s\' and listing_ip = '%s'",cluster_id, server_name_esc, listing_ip);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		free(server_name_esc);
		return(YP_ERROR);
	}

	free(server_name_esc);
	server_name_esc = NULL;

	nrows = mysql_affected_rows(&dbase);
	
	if(nrows == 0) {
		strcpy(error, "update of cluster_id for clustered servers failed");
		return(YP_ERROR);
	}

	if(mysql_real_query(&dbase,sql2,strlen(sql2))) {
		strcpy(error, mysql_error(&dbase));
		free(server_name_esc);
		return(YP_ERROR);
	}
	nrows = mysql_affected_rows(&dbase);
	
	if(nrows == 0) {
		strcpy(error, "update of cluster_id for servers failed");
		return(YP_ERROR);
	}
	return(1);
}

int addServer(char *server_name, char *genre, char *cluster_password, char *desc, char *url, char *listenurl, char *server_type, char *bitrate, char *listing_ip, char *sid, char *samplerate, char *channels, char *error)
{
	char	sql[8096];
	int	i;
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
	char	*server_name_esc = NULL;
	char	*genre_esc = NULL;
	char	*desc_esc = NULL;
	char	*url_esc = NULL;
	char	*server_type_esc = NULL;
	char	*bitrate_esc = NULL;
	char	*listenurl_esc = NULL;
	char	*listeners_esc = NULL;
	char	*samplerate_esc = NULL;
	char	*channels_esc = NULL;
	int	cluster_flag = 0;
	int	have_cluster_id = 0;
	char	table_name[255] = "";
	char	parent_id[255] = "";
	char	detail_id[255] = "";
	int	ret = 0;

	/* Check for Dupes */
	memset(sql, '\000', sizeof(sql));

	server_name_esc = malloc(strlen(server_name)*2 + 1);
	memset(server_name_esc, '\000', strlen(server_name)*2 + 1);
	mysql_real_escape_string(&dbase, server_name_esc, server_name, strlen(server_name));

	listenurl_esc = malloc(strlen(listenurl)*2 + 1);
	memset(listenurl_esc, '\000', strlen(listenurl)*2 + 1);
	mysql_real_escape_string(&dbase, listenurl_esc, listenurl, strlen(listenurl));

	Log(LOG_DEBUG, "Going to execute %s", sql);
	snprintf(sql, sizeof(sql)-1, "select count(*) from servers a, server_details b where a.server_name = \'%s\' and b.listen_url = '%s' and a.id = b.parent_id", server_name_esc, listenurl_esc);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		goto Error;
	}

	free(server_name_esc);
	free(listenurl_esc);
	server_name_esc = NULL;
	listenurl_esc = NULL;

	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	if(nrows == 0) {
		Log(LOG_DEBUG, "Num rows for the count(*) is 0..hmmm");
		goto Error;
	}
	else {
		row = mysql_fetch_row(result);
		mysql_free_result(result);
		if (row[0]) {
			existing = atoi(row[0]);
		}
		if (existing) {
			return(YP_EXISTS);
		}
	}

	memset(sql, '\000', sizeof(sql));

	Log(LOG_DEBUG, "Checking for a cluster");
	existing = checkForCluster(server_name, listing_ip, error, parent_id);
	if (existing == YP_ERROR) {
		Log(LOG_DEBUG, "check for cluster returned error");
		goto Error;
	}
	if (existing) {
		cluster_flag = 1;
	}
	Log(LOG_DEBUG, "Done Checking for a cluster");

	if (cluster_flag) {
		strcpy(table_name, "clustered_servers");
		Log(LOG_DEBUG, "Cluster flag is set!");
	}
	else {
		strcpy(table_name, "servers");
		Log(LOG_DEBUG, "Cluster flag is NOT set!");
	}

	if (!strcmp(server_name, "Example stream name")) {
		sprintf(error, "Bad stream name, be more original");
		goto Error;
	}

	server_name_esc = malloc(strlen(server_name)*2 + 1);
	memset(server_name_esc, '\000', strlen(server_name)*2 + 1);
	genre_esc = malloc(strlen(genre)*2 + 1);
	memset(genre_esc, '\000', strlen(genre)*2 + 1);
	url_esc = malloc(strlen(url)*2 + 1);
	memset(url_esc, '\000', strlen(url)*2 + 1);
	desc_esc = malloc(strlen(desc)*2 + 1);
	memset(desc_esc, '\000', strlen(desc)*2 + 1);
	server_type_esc = malloc(strlen(server_type)*2 + 1);
	memset(server_type_esc, '\000', strlen(server_type)*2 + 1);
	bitrate_esc = malloc(strlen(bitrate)*2 + 1);
	memset(bitrate_esc, '\000', strlen(bitrate)*2 + 1);
	listenurl_esc = malloc(strlen(listenurl)*2 + 1);
	memset(listenurl_esc, '\000', strlen(listenurl)*2 + 1);
	samplerate_esc = malloc(strlen(samplerate)*2 + 1);
	memset(samplerate_esc, '\000', strlen(samplerate)*2 + 1);
	channels_esc = malloc(strlen(channels)*2 + 1);
	memset(channels_esc, '\000', strlen(channels)*2 + 1);

	mysql_real_escape_string(&dbase, server_name_esc, server_name, strlen(server_name));
	mysql_real_escape_string(&dbase, genre_esc, genre, strlen(genre));
	mysql_real_escape_string(&dbase, url_esc, url, strlen(url));
	mysql_real_escape_string(&dbase, desc_esc, desc, strlen(desc));
	mysql_real_escape_string(&dbase, server_type_esc, server_type, strlen(server_type));
	mysql_real_escape_string(&dbase, bitrate_esc, bitrate, strlen(bitrate));
	mysql_real_escape_string(&dbase, listenurl_esc, listenurl, strlen(listenurl));
	mysql_real_escape_string(&dbase, samplerate_esc, samplerate, strlen(samplerate));
	mysql_real_escape_string(&dbase, channels_esc, channels, strlen(channels));


	if (!cluster_flag) {
		snprintf(sql, sizeof(sql)-1, "insert into servers (server_name, listing_ip) values  ('%s', '%s')", server_name_esc, listing_ip);
		Log(LOG_DEBUG, "This isn't a cluster, it's just a new stream");
		if (mysql_real_query(&dbase,sql,strlen(sql))) {
			sprintf(error, "servers: %s", mysql_error(&dbase));
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			goto Error;
		}
		sprintf(parent_id, "%d", mysql_insert_id(&dbase));
	}

	Log(LOG_DEBUG, sql);

	snprintf(sql, sizeof(sql)-1, "insert into \
		server_details (parent_id,		\
			  server_name,		\
			  listing_ip,		\
			  description,		\
			  genre,		\
			  sid,			\
			  cluster_password,	\
			  url,			\
			  current_song,		\
			  listen_url,		\
			  server_type,		\
			  bitrate,		\
			  listeners,		\
			  samplerate,		\
			  channels)		\
		 values  (%s,			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  '%s',			\
			  0,			\
			  '%s',			\
			  '%s')", parent_id, server_name_esc, listing_ip, desc_esc, genre_esc, sid, cluster_password, url_esc, "", listenurl_esc, server_type_esc, bitrate_esc, samplerate_esc, channels_esc);
	if (mysql_real_query(&dbase,sql,strlen(sql))) {
		sprintf(error, "servers: %s", mysql_error(&dbase));
		sprintf(sql,"ROLLBACK");
		mysql_real_query(&dbase,sql,strlen(sql));
		goto Error;
	}
	Log(LOG_DEBUG, "We have a parent id, which means this is part of a cluster that is currently in operation");
	
	// If we didn't have a cluster id before, this means that
	// we need up update the record in clustered_servers to reflect the newly generated cluster id
	sprintf(detail_id, "%d", mysql_insert_id(&dbase));
	
	sprintf(sid, "%s-%s", parent_id, detail_id);

	setLogSid(sid);

	Log(LOG_DEBUG, "Going to insert into servers_touch now...");
	sprintf(sql,"insert into servers_touch (id, server_name, listing_ip, last_touch) values  ('%s', '%s', '%s', NOW())", sid, server_name_esc, listing_ip);

	Log(LOG_DEBUG, sql);

	if (mysql_real_query(&dbase,sql,strlen(sql))) {
		sprintf(error, "servers_info: %s", mysql_error(&dbase));
		sprintf(sql,"ROLLBACK");
		mysql_real_query(&dbase,sql,strlen(sql));
		return(YP_ERROR);
	}
	sprintf(sql,"COMMIT");
	mysql_real_query(&dbase,sql,strlen(sql));
	Log(LOG_DEBUG, "Everything is good");
	if (server_name_esc) {
		free(server_name_esc);
	}
	if (desc_esc) {
		free(desc_esc);
	}
	if (genre_esc) {
		free(genre_esc);
	}
	if (url_esc) {
		free(url_esc);
	}
	if (server_type_esc) {
		free(server_type_esc);
	}
	if (bitrate_esc) {
		free(bitrate_esc);
	}
	if (listenurl_esc) {
		free(listenurl_esc);
	}
	if (samplerate_esc) {
		free(samplerate_esc);
	}
	if (channels_esc) {
		free(channels_esc);
	}
	return(YP_ADDED);
Error:
	if (server_name_esc) {
		free(server_name_esc);
	}
	if (desc_esc) {
		free(desc_esc);
	}
	if (genre_esc) {
		free(genre_esc);
	}
	if (url_esc) {
		free(url_esc);
	}
	if (server_type_esc) {
		free(server_type_esc);
	}
	if (bitrate_esc) {
		free(bitrate_esc);
	}
	if (listenurl_esc) {
		free(listenurl_esc);
	}
	if (samplerate_esc) {
		free(samplerate_esc);
	}
	if (channels_esc) {
		free(channels_esc);
	}
	return(YP_ERROR);

}


int touchServer(char *sid, char *touchip, char *cluster_password, char *song, char *listeners, char *genre, char *desc, char *listenurl, char *server_type, char *bitrate, char *server_name, char *error, int touchType)
{
	char	sql[8096];
	int	i;
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
	char	*song_esc = NULL;
	char	*genre_esc = NULL;
	char	*listenurl_esc = NULL;
	char	*server_type_esc = NULL;
	char	*bitrate_esc = NULL;
	char	*desc_esc = NULL;
	char	*server_name_esc = NULL;
	int	cluster_flag = 0;
	char	detail_id[255] = "";
	char	parent_id[255] = "";
	char	*p1;

	memset(sql, '\000', sizeof(sql));

	memset(detail_id, '\000', sizeof(detail_id));
	memset(parent_id, '\000', sizeof(parent_id));
	p1 = strchr(sid, '-');
	if (p1) {
		cluster_flag = 1;
		strncpy(parent_id, sid, p1-sid);
		strcpy(detail_id, p1+1);
	}

	sprintf(sql,"select listing_ip from servers_touch where id = \'%s\'",sid);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		goto TouchError;
	}
	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	if(nrows == 0) {
		goto TouchError;
	}
	else {
		row = mysql_fetch_row(result);
		if (row[0]) {
			if (strcmp(touchip, row[0])) {
				strcpy(error, "trying to touch from a different IP than was added");
				return(YP_ERROR);
			}
		}
		mysql_free_result(result);

		song_esc = malloc(strlen(song)*2 + 1);
		memset(song_esc, '\000', strlen(song)*2 + 1);
		mysql_real_escape_string(&dbase, song_esc, song, strlen(song));

		sprintf(sql,"update server_details set current_song = \'%s\', listeners = %d where id = %s", song_esc, atol(listeners), detail_id);
	
		Log(LOG_DEBUG, sql);
		if (mysql_real_query(&dbase,sql,strlen(sql))) {
			sprintf(error, "servers: %s", mysql_error(&dbase));
			Log(LOG_ERROR, error);
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			goto TouchError;
		}
		if (mysql_affected_rows(&dbase) < 0) {
			// something squirly...
			sprintf(error, "Error updating server info, %d records updated", mysql_affected_rows(&dbase));
			Log(LOG_ERROR, error);
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			goto TouchError;
		}

		sprintf(sql,"update servers_touch set last_touch = NOW() where id = '%s'", sid);

		Log(LOG_DEBUG, sql);
		
		if (mysql_real_query(&dbase,sql,strlen(sql))) {
			sprintf(error, "servers_touch: %s", mysql_error(&dbase));
			Log(LOG_ERROR, error);
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			goto TouchError;
		}
		if (mysql_affected_rows(&dbase) < 0) {
			// something squirly...
			sprintf(error, "Error updating server touch info, %d records updated", mysql_affected_rows(&dbase));
			Log(LOG_ERROR, error);
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			goto TouchError;
		}
		sprintf(sql,"COMMIT");
		mysql_real_query(&dbase,sql,strlen(sql));


		sprintf(sql,"select sum(listeners) from server_details where parent_id = %s",parent_id);
		if(mysql_real_query(&dbase,sql,strlen(sql))) {
			strcpy(error, mysql_error(&dbase));
			goto TouchError;
		}
		result = mysql_store_result(&dbase);
		nrows = mysql_num_rows(result);
		if(nrows == 0) {
			goto TouchError;
		}
		else {
			row = mysql_fetch_row(result);
			if (row[0]) {
				sprintf(sql,"update servers set listeners = %s where id = %s", row[0], parent_id);

				Log(LOG_DEBUG, sql);
				
				if (mysql_real_query(&dbase,sql,strlen(sql))) {
					sprintf(error, "servers_touch: %s", mysql_error(&dbase));
					Log(LOG_ERROR, error);
					sprintf(sql,"ROLLBACK");
					mysql_real_query(&dbase,sql,strlen(sql));
					goto TouchError;
				}
			}
		}

		mysql_free_result(result);

		Log(LOG_DEBUG, "YP Touched");
		if (song_esc) {
			free(song_esc);
		}
		if (genre_esc) {
			free(genre_esc);
		}
		if (listenurl_esc) {
			free(listenurl_esc);
		}
		if (server_type_esc) {
			free(server_type_esc);
		}
		if (bitrate_esc) {
			free(bitrate_esc);
		}
		if (desc_esc) {
			free(desc_esc);
		}
		if (server_name_esc) {
			free(server_name_esc);
		}
		return(YP_TOUCHED);
	}
TouchError:
	if (song_esc) {
		free(song_esc);
	}
	if (genre_esc) {
		free(genre_esc);
	}
	if (listenurl_esc) {
		free(listenurl_esc);
	}
	if (server_type_esc) {
		free(server_type_esc);
	}
	if (bitrate_esc) {
		free(bitrate_esc);
	}
	if (desc_esc) {
		free(desc_esc);
	}
	if (server_name_esc) {
		free(server_name_esc);
	}
	return(YP_ERROR);

}
int removeServer(char *sid, char *removeip, char *cluster_password, char *error)
{
	char	sql[8096];
	int	i;
	MYSQL_ROW	row;
	int	existing = 0;
	int	nrows = 0;
	int	cluster_flag = 0;
	char	detail_id[255] = "";
	char	parent_id[255] = "";
	char	*p1;

	memset(sql, '\000', sizeof(sql));

	memset(detail_id, '\000', sizeof(detail_id));
	memset(parent_id, '\000', sizeof(parent_id));
	p1 = strchr(sid, '-');
	if (p1) {
		cluster_flag = 1;
		strncpy(parent_id, sid, p1-sid);
		strcpy(detail_id, p1+1);
	}

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"select listing_ip from servers_touch where id = '%s'",sid);
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		return(YP_ERROR);
	}
	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	if(nrows == 0) {
		return(YP_ERROR);
	}
	else {
		row = mysql_fetch_row(result);
		if (row[0]) {
			if (strcmp(removeip, row[0])) {
				strcpy(error, "trying to remove from a different IP than was added");
				return(YP_ERROR);
			}
		}
		mysql_free_result(result);

		sprintf(sql,"delete from server_details where id = %s", detail_id);
		if (mysql_real_query(&dbase,sql,strlen(sql))) {
			sprintf(error, "clustered_servers: %s", mysql_error(&dbase));
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			return(YP_ERROR);
		}
	
		// At this point, we need to check to see if there are any server_details left for this
		// parent, if not, then delete the parent, if so, then leave the parent, because 
		// this is a cluster.

		Log(LOG_DEBUG, "Checking to see if there are any more server details");
		if (!anyMoreServerDetails(sid, error, parent_id)) {
			Log(LOG_DEBUG, "Nope, so lets delete the parent...");
			// If we are deleting the parent cluster, then whack all other clustered servers
			// this is a bit crappy, but the only way to preserve the RI...
			// Now go through and delete all server_touch records for the records we are
			// about to whack in clustered_servers

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
	
		sprintf(sql,"delete from servers_touch where id = '%s'", sid);
		
		if (mysql_real_query(&dbase,sql,strlen(sql))) {
			sprintf(error, "servers_info: %s", mysql_error(&dbase));
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			return(YP_ERROR);
		}
		if (mysql_affected_rows(&dbase) < 0) {
			// something squirly...
			sprintf(error, "Error removing server info, %d records deleted", mysql_affected_rows(&dbase));
			sprintf(sql,"ROLLBACK");
			mysql_real_query(&dbase,sql,strlen(sql));
			return(YP_ERROR);
		}
		sprintf(sql,"COMMIT");
		mysql_real_query(&dbase,sql,strlen(sql));
		return(YP_TOUCHED);
	}
	return(YP_ERROR);

}

void sendOK()
{
	printf("HTTP/1.0 200 OK\r\n");

}
void sendHTML(char *html)
{
	static int	headersent = 0;

	if (!headersent) {
		printf("Content-type: text/html\n\n");
		headersent = 1;
	}
	printf("%s", html);
}
void sendYPResponse(int errorcode, char *msg, int type)
{
	if (errorcode == 0) {
		Log(LOG_INFO, msg);
	}
	if (type == AUDIOCAST_RESPONSE) {
		if (errorcode != 1) {
			printf("x-audiocast-yp-error: %d %s\r\n\r\n", errorcode, msg);
		}
	}
	if (type == ICECAST2_RESPONSE) {
		if (strlen(msg) == 0) {
			printf("YPResponse: 0\r\nYPMessage: NAK\r\n\r\n", errorcode);
			Log(LOG_DEBUG, "Sending NAK");
		}
		else {
			printf("YPResponse: %d\r\nYPMessage: %s\r\n\r\n", errorcode, msg);
		}
	}
}


