/* oggmerge.h
**
*/

#ifndef __OGGMERGE_H__
#define __OGGMERGE_H__

#include <ogg/ogg.h>

typedef struct {
	ogg_page *og;
	u_int64_t timestamp;
} oggmerge_page_t;

typedef struct {
	void *private;
} oggmerge_state_t;

typedef struct filelist_tag {
	char *name;
	FILE *fp;

	int type;

	oggmerge_state_t state;
	oggmerge_page_t *page;
	int status;
	unsigned serialno;

	int (*state_init)(oggmerge_state_t *state, int serialno);
	int (*data_in)(oggmerge_state_t *state, char *buffer, unsigned long size);
	oggmerge_page_t *(*page_out)(oggmerge_state_t *state);

	struct filelist_tag *next;
} filelist_t;

typedef struct {
	char *outfile;
	FILE *out;
	filelist_t *input;
	int quiet;
	int verbose;
} param_t;

/* errors */
#define EMOREDATA -1
#define EMALLOC -2
#define EBADHEADER -3
#define EBADEVENT -4
#define EOTHER -5

/* types */
#define TYPEUNKNOWN 0
#define TYPEVORBIS 1
#define TYPEMIDI 2
#define TYPEMNG 3

#endif  /* __OGGMERGE_H__ */





