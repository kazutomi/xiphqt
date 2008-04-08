/* midi.h
**
** oggmerge midi file module
**
*/

#ifndef __MIDI_H__
#define __MIDI_H__

#include "oggmerge.h"

int midi_state_init(oggmerge_state_t *state, int serialno, int old_style);
int midi_data_in(oggmerge_state_t *state, char *buffer, unsigned long size);
oggmerge_page_t *midi_page_out(oggmerge_state_t *state);
int midi_fisbone_out(oggmerge_state_t *state, ogg_packet *op);

#endif  /* __MIDI_H__*/
