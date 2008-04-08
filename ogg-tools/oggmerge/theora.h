/* theora.h
**
** oggmerge theora file module
**
*/

#ifndef __THEORA_H__
#define __THEORA_H__

#include "oggmerge.h"

int theora_state_init(oggmerge_state_t *state, int serialno, int old_style);
int theora_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *theora_page_out(oggmerge_state_t *state);
int theora_fisbone_out(oggmerge_state_t *state, ogg_packet *op);

#endif  /* __THEORA_H__*/
