#ifndef pnmio_h
#define pnmio_h


#define PNM_BITMAP '1'
#define PNM_GRAYSCALE '2'
#define PNM_PIXMAP '3'
#define PNM_RAW_BITMAP '4'
#define PNM_RAW_GRAYSCALE '5'
#define PNM_RAW_PIXMAP '6'

int read_pnm_header(FILE *fp, size_t *type, size_t *w, size_t *h, size_t *maxval);
int write_pnm_header(FILE *fp, size_t type, size_t w, size_t h, size_t maxval, char *comment);
int read_ppm(FILE *fp, char *buf,size_t *w, size_t *h, size_t *maxval);
int read_pgm(FILE *fp, char *buf,size_t *w, size_t *h, size_t *maxval);
#endif
