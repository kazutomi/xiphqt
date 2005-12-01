

#ifndef _CGI_H
#define _CGI_H
#define QRY_MAXPARAMS 64
struct key_value {
  char *key;
  char *value;
};

struct key_value gparams[QRY_MAXPARAMS+1];

int parseQuery(struct key_value * params, char * qry);
char * paramValue(struct key_value * params, char * skey); // returns pointer to parameter value, NULL if not defined
int unescape (char * s, int l);
int hexdigit (char c);


#endif
