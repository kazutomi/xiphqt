/*********************************************
YP-CACHEGEN by oddsock
**********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <unistd.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <iconv.h>
#include "log.h"


extern MYSQL		dbase;
MYSQL_RES	*result;
MYSQL_RES	*result2;

#define ERROR -1
#define SUCCESS 1

unsigned char*
convert (unsigned char *in, char *encoding)
{
	char *out, *pin, *pout;
        int ret,size,out_size,temp;
	iconv_t	iconv_handle;
	int i =0;


        size = (int)strlen(in)+1; 
        out_size = size*2-1; 
        out = malloc((size_t)out_size); 

	pin = (char *)in;
	pout = out;

        if (out) {
		iconv_handle =  iconv_open("UTF-8", "ISO-8859-1");
        }
        if (out) {
		size_t ret = iconv(iconv_handle, &pin, &size, &pout, &out_size);
		if (ret == -1) {
			printf("unable to convert (%s)", in);
		}
		iconv_close(iconv_handle);
        } else {
                printf("no mem\n");
        }
	for (i=0;i<out_size;i++) {
		if ((out[i] < 16)) {
			out[i] = '.';
		}
	}
        return (out);
}	


int gen_cache(char *error)
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
	char *encoding = "UTF-8";

	xmlDocPtr doc;
	xmlNodePtr rootNode;
	xmlNodePtr entryNode;

	doc = xmlNewDoc("1.0");
	rootNode = xmlNewDocNode(doc, NULL, "directory", NULL);
	xmlDocSetRootElement(doc, rootNode);


	srand(time() + getpid());

	memset(sql, '\000', sizeof(sql));

	sprintf(sql,"select a.server_name, b.listen_url, b.server_type, b.bitrate, b.channels, b.samplerate, b.genre, b.current_song from servers a, server_details b where a.id = b.parent_id and yp_status = 'verified' order by server_name");
	if(mysql_real_query(&dbase,sql,strlen(sql))) {
		strcpy(error, mysql_error(&dbase));
		return(ERROR);
	}
	result = mysql_store_result(&dbase);
	nrows = mysql_num_rows(result);
	if(nrows == 0) {
		return(SUCCESS);
	}
	else {
		for (i=0;i<nrows;i++) {
			row = mysql_fetch_row(result);
			if (row[0]) {
				unsigned char *out0;
				unsigned char *out1;
				unsigned char *out2;
				unsigned char *out3;
				unsigned char *out4;
				unsigned char *out5;
				unsigned char *out6;
				unsigned char *out7;

				entryNode = xmlNewChild(rootNode, NULL, "entry", NULL);
				out0 = convert(row[0], encoding);
				out1 = convert(row[1], encoding);
				out2 = convert(row[2], encoding);
				out3 = convert(row[3], encoding);
				out4 = convert(row[4], encoding);
				out5 = convert(row[5], encoding);
				out6 = convert(row[6], encoding);
				out7 = convert(row[7], encoding);

				xmlNewTextChild(entryNode, NULL, "server_name", out0);
				xmlNewTextChild(entryNode, NULL, "listen_url", out1);
				xmlNewTextChild(entryNode, NULL, "server_type", out2);
				xmlNewTextChild(entryNode, NULL, "bitrate", out3);
				xmlNewTextChild(entryNode, NULL, "channels", out4);
				xmlNewTextChild(entryNode, NULL, "samplerate", out5);
				xmlNewTextChild(entryNode, NULL, "genre", out6);
				xmlNewTextChild(entryNode, NULL, "current_song", out7);
				free(out0);
				free(out1);
				free(out2);
				free(out3);
				free(out4);
				free(out5);
				free(out6);
				free(out7);
			}
		}
		mysql_free_result(result);
	}
	unlink("yp.xml");
	xmlSaveFormatFileEnc("yp.xml", doc, encoding, 1);
	return(SUCCESS);
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
	setLogFile(YP_LOGDIR"yp-cachegen.log");

	if (connectToDB()) {
		ret = gen_cache(error);
		if (ret == ERROR) {
			LogMessage(LOG_ERROR, "Error: %s", error);
		}
	}
	else {
		LogMessage(LOG_ERROR, "Cannot connect to DB!");
	}
	return(0);
}


