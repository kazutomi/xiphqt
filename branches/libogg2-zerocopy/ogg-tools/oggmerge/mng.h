/* mng.h
**
** oggmerge mng file module
**
*/

#ifndef __MNG_H__
#define __MNG_H__

#include "oggmerge.h"

int mng_state_init(oggmerge_state_t *state, int serialno);
int mng_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *mng_page_out(oggmerge_state_t *state);

#endif  /* __MNG_H__*/
