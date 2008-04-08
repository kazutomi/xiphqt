/* skeleton.h
**
** oggmerge skeleton file module
**
*/

#ifndef __SKELETON_H__
#define __KELETON_H__

#include "config.h"

#include "oggmerge.h"

int skeleton_state_init(oggmerge_state_t *state, int serialno, int old_style);
int skeleton_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *skeleton_page_out(oggmerge_state_t *state);
int skeleton_fisbone_out(oggmerge_state_t *state, ogg_packet *op);

enum { FISHEAD, FISBONE, FISHTAIL };

int skeleton_packetin(oggmerge_state_t *state, ogg_packet *op, int type);

#endif  /* __SKELETON_H__*/
