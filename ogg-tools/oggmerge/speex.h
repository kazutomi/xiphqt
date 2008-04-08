/* speex.h
**
** oggmerge speex file module
**
*/

#ifndef __SPEEX_H__
#define __SPEEX_H__

#include "config.h"

#include "oggmerge.h"

int speex_state_init(oggmerge_state_t *state, int serialno, int old_style);
int speex_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *speex_page_out(oggmerge_state_t *state);
int speex_fisbone_out(oggmerge_state_t *state, ogg_packet *op);

#endif  /* __SPEEX_H__*/
