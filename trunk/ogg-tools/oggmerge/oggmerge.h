/*
	oggmerge -- utility for splicing together ogg bitstreams
			from component media subtypes

	oggmerge.h

	Copyright 2000 Ralph Giles <Ralph_Giles@telus.net>

	Distributed under the GPL
	see the file COPYING for details
	or visit http://www.gnu.org/copyleft/gpl.html
*/

#ifndef OGGMERGE_H
#define OGGMERGE_H

#include <ogg/ogg.h>

/* state storage structure */
typedef struct {
	char	*infile, *outfile;
	FILE	*in,*out;
	int	quiet,verbose;
	ogg_stream_state *os;
	ogg_sync_state *oy;
} param_t;


#endif /* OGGMERGE_H */
