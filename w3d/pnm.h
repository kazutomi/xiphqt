#ifndef __PPM_H
#define __PPM_H


/**
 *   Returns number of color channels (1 == grayscale, 3 == rgb)
 *   or -1 on error.
 */
extern int read_pnm_header (char *fname, int *w, int *h);

/**
 *   Read pnm into buf. Assumes you have called read_pnm_header()
 *   and allocated buf before.
 */
extern int read_pnm (char *fname, uint8_t *buf);

/**
 *   Write buf into pnm file. Depending of the suffix of fname we
 *   assume 1 channel for '.png' and 3 channels for '.ppm'
 */
extern void write_pnm (char *fname, uint8_t *buf, int w, int h);

/**
 *   Write a int16_t buf into pgm file
 */
extern void write_pgm16 (char *fname, int16_t *buf, int w, int h);

#endif

