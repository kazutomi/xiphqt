#ifndef __TARKIN_IO_H
#define __TARKIN_IO_H

#include "tarkin.h"

extern TarkinStream * tarkin_stream_new();
void                  tarkin_stream_destroy (TarkinStream *s);

/* free_frame() is called anytime the codec encoded a frame,
 * so the caller application can free it. */
void tarkin_stream_encinit(void (*free_frame)(uint8_t **buf));

extern
TarkinError tarkin_stream_write_layer_descs (TarkinStream *s,
                                             uint32_t n_layers,
                                             TarkinVideoLayerDesc desc []);

uint32_t tarkin_stream_write(TarkinStream *s, uint8_t **buf);
int tarkin_stream_packetout(TarkinStream *s, ogg_packet *op);

int tarkin_stream_headerin();
int tarkin_stream_packetin();
extern int write_tarkin_header (int fd, TarkinStream *s);
extern int write_layer_descs (int fd, TarkinStream *s);
extern int write_tarkin_bitstream (int fd, uint8_t *bitstream, uint32_t len);

extern int read_tarkin_header (int fd, TarkinStream *s);
extern int read_layer_descs (int fd, TarkinStream *s);
extern int read_tarkin_bitstream (int fd, uint8_t *bitstream);

#endif

