#ifndef __TARKIN_IO_H
#define __TARKIN_IO_H

#include "tarkin.h"

extern int write_tarkin_header (int fd, TarkinStream *s);
extern int write_layer_descs (int fd, TarkinStream *s);
extern int write_tarkin_bitstream (int fd, uint8_t *bitstream, uint32_t len);

extern int read_tarkin_header (int fd, TarkinStream *s);
extern int read_layer_descs (int fd, TarkinStream *s);
extern int read_tarkin_bitstream (int fd, uint8_t *bitstream);

#endif

