/* oggmerge.h
**
*/

#ifndef __OGGMERGE_H__
#define __OGGMERGE_H__

#include <ogg/ogg.h>

typedef struct {
	ogg_page *og;
	int64_t timestamp;
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
	int fisbone_done;

	int (*state_init)(oggmerge_state_t *state, int serialno, int old_style);
	int (*data_in)(oggmerge_state_t *state, char *buffer, unsigned long size);
	oggmerge_page_t *(*page_out)(oggmerge_state_t *state);
	int (*fisbone_out)(oggmerge_state_t *state, ogg_packet *op);

	struct filelist_tag *next;
} filelist_t;

typedef struct {
	char *outfile;
	FILE *out;
	filelist_t *input;
	int verbose;
        int old_style;
        int skeleton;
} param_t;

/* errors */
#define EMOREDATA -1
#define EMALLOC -2
#define EBADHEADER -3
#define EBADEVENT -4
#define EOTHER -5

/* types */
#define TYPESKELETON (-1)
#define TYPEUNKNOWN 0
#define TYPEVORBIS 1
#define TYPEMIDI 2
#define TYPEMNG 3
#define TYPEKATE 4
#define TYPETHEORA 5
#define TYPESPEEX 6

extern void add_fisbone_packet (ogg_packet *op,
                                ogg_uint32_t serial,
                                const char *content_type, int headers, int preroll,
                                int gshift, ogg_int64_t gnum, ogg_int64_t gden);

#endif  /* __OGGMERGE_H__ */
