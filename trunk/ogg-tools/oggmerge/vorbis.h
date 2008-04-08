/* vorbis.h
**
** oggmerge vorbis file module
**
*/

#ifndef __VORBIS_H__
#define __VORBIS_H__

#include "oggmerge.h"

int vorbis_state_init(oggmerge_state_t *state, int serialno, int old_style);
int vorbis_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *vorbis_page_out(oggmerge_state_t *state);
int vorbis_fisbone_out(oggmerge_state_t *state, ogg_packet *op);

#endif  /* __VORBIS_H__*/
