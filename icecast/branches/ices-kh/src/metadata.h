/* metadata.h
 * - metadata stuff.
 *
 * $Id: metadata.h,v 1.3 2001/09/25 12:04:22 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __METADATA_H__
#define __METADATA_H__

#include "inputmodule.h"

extern int metadata_update_signalled;

void *metadata_thread_signal(void *arg, input_buffer *buffer);
void metadata_update(char **md, vorbis_comment *vc);
void metadata_free (char **md);


#endif /* __METADATA_H__ */

