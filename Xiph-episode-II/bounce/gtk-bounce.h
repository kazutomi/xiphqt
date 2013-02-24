#define _GNU_SOURCE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#ifndef _REENTRANT
# define _REENTRANT
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>

extern sig_atomic_t exiting;
extern int eventpipe[2];

extern void *io_thread(void *dummy);

extern sig_atomic_t request_bits;
//extern sig_atomic_t request_ch;
extern sig_atomic_t request_rate;

extern sig_atomic_t request_ch1_quant;
extern sig_atomic_t request_ch2_quant;
extern sig_atomic_t prime_1kHz_notch;
extern sig_atomic_t request_1kHz_notch;
extern sig_atomic_t request_1kHz_sine;
extern sig_atomic_t request_5kHz_sine;
extern sig_atomic_t request_1kHz_sine2;
extern sig_atomic_t request_1kHz_amplitude;
extern sig_atomic_t request_1kHz_modulate;
extern sig_atomic_t request_1kHz_notch;
extern sig_atomic_t request_dither;
extern sig_atomic_t request_dither_shaped;
extern sig_atomic_t request_dither_amplitude;
extern sig_atomic_t request_output_noise;
extern sig_atomic_t request_output_sweep;
extern sig_atomic_t request_output_logsweep;
extern sig_atomic_t request_output_silence;

extern sig_atomic_t request_lp_filter;
extern sig_atomic_t request_1kHz_square;
extern sig_atomic_t request_1kHz_square_offset;
extern sig_atomic_t request_1kHz_square_spread;
extern sig_atomic_t request_output_tone;
extern sig_atomic_t request_output_duallisten;

#define fromdB(x) (exp((x)*.11512925f))
