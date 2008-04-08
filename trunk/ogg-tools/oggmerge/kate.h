/* kate.h
**
** oggmerge kate file module
**
*/

#ifndef __KATE_H__
#define __KATE_H__

#include "config.h"

#include "oggmerge.h"

int kate_state_init(oggmerge_state_t *state, int serialno, int old_style);
int kate_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *kate_page_out(oggmerge_state_t *state);
int kate_fisbone_out(oggmerge_state_t *state, ogg_packet *op);

#endif  /* __KATE_H__*/
