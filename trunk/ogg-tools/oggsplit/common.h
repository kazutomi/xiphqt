/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#ifndef _XMALLOC_H
#define _XMALLOC_H

void *xmalloc(size_t size);
void *xrealloc(void *p, size_t size);
char *xstrdup(const char *source);

#endif /* _XMALLOC_H */
