#ifndef __PPM_H
#define __PPM_H


extern int read_ppm_info (char *fname, int *w, int *h);
extern int read_ppm (char *fname, uint8_t *buf, int w, int h);

extern void write_ppm (char *fname, uint8_t *buf, int w, int h);
extern void write_ppm16 (char *fname, int16_t *buf, int w, int h);

#endif

