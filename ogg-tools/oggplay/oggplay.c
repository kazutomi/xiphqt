/*
	oggplay

	a testbed player for ogg multimedia

	$Date: 2001/08/25 22:22:31 $

	Ralph Giles <giles@thaumas.net>

	This program my be redistributed under the terms of the
	GNU General Public Licence, version 2, or at your preference,
	any later version.
*/

#include <stdio.h>
#include <stdlib.h>

#include <ogg/ogg.h>
#include <libmng.h>
#include <SDL/SDL.h>

#ifndef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

/* structure for keeping track of our mng stream inside the callbacks */
typedef struct {
	FILE		*file;	   /* pointer to the file we're decoding */
	char		*filename; /* pointer to the file path/name */
	ogg_sync_state   *oy;	   /* ogg input stream machinery */
	ogg_stream_state *os;
        ogg_page	 *og_last; /* for saving state */
	ogg_packet	 *op_last;
        int		 op_bytes;
	SDL_Surface	*surface;  /* SDL display */
	mng_uint32	delay;     /* ticks to wait before resuming decode */
} stuff_t;

/* callbacks for the mng decoder */

/* memory allocation; data must be zeroed */
mng_ptr mymngalloc(mng_uint32 size)
{
	return (mng_ptr)malloc(size);
}

/* memory deallocation */
void mymngfree(mng_ptr p, mng_uint32 size)
{
	free(p);
	return;
}

mng_bool mymngopenstream(mng_handle mng)
{
	stuff_t	*stuff;

	/* look up our stream struct */
        stuff = (stuff_t*)mng_get_userdata(mng);
	
	/* open the file */
	stuff->file = fopen(stuff->filename, "rb");
	if (stuff->file == NULL) {
		fprintf(stderr, "unable to open '%s'\n", stuff->filename);
		return MNG_FALSE;
	}

	/* initialize our ogg decoder */
	if (stuff->oy == NULL) {
		stuff->oy = (ogg_sync_state*)malloc(sizeof(ogg_sync_state));
		if (stuff->oy == NULL) {
			fprintf(stderr, "unable to allocate ogg sync state\n");
 			fclose(stuff->file);
			return MNG_FALSE;
		}
		ogg_sync_init(stuff->oy);
	} else {
		fprintf(stderr, "warning: reusing old ogg sync state\n");
  		ogg_sync_reset(stuff->oy);
	}
	/* we'll set this part up once we find our substream */
	if (stuff->os != NULL) {
		fprintf(stderr, "warning: reusing old ogg stream state\n");
 		ogg_stream_destroy(stuff->os);
		stuff->os = NULL;
	}
		
	return MNG_TRUE;
}

mng_bool mymngclosestream(mng_handle mng)
{
	stuff_t	*stuff;

	/* look up our stream struct */
        stuff = (stuff_t*)mng_get_userdata(mng);

	/* free the ogg state */
	if (stuff->os != NULL) {
		ogg_stream_destroy(stuff->os);
		stuff->os = NULL;
	}
	if (stuff->oy != NULL) {
		ogg_sync_destroy(stuff->oy);
		stuff->oy = NULL;
	}

	/* close the file */
	fclose(stuff->file);
	stuff->file = NULL;	/* for safety */

	return MNG_TRUE;
}

/* feed data to the decoder */
mng_bool mymngreadstream(mng_handle mng, mng_ptr buffer,
		mng_uint32 byteswanted, mng_uint32 *bytesread)
{
	stuff_t *stuff;
	char	*buf;
	int	rc;
	int	   streambytes, copybytes;
	ogg_page   *og = calloc(sizeof(*og),1);
	ogg_packet *op = calloc(sizeof(*op),1);

	/* look up our stream struct */
	stuff = (stuff_t*)mng_get_userdata(mng);

	*bytesread = 0;

	fprintf(stderr, "trying to get %d bytes for libmng\n", byteswanted);

	/* do we have any packets/pages left over from the last call? */
	if (stuff->og_last && stuff->op_last) {
		fprintf(stderr, "found packet %ld from last time\n",
			stuff->op_last->packetno);
			copybytes = MIN(byteswanted, stuff->op_last->bytes - stuff->op_bytes);
			fprintf(stderr, "  submitting %d bytes to the mng decoder\n", copybytes);

			/* copy it into the mng decode buffer */
			memcpy(buffer, stuff->op_last->packet, copybytes);
			*bytesread += copybytes;
	}


	/* get a decoding buffer */
	buf = ogg_sync_buffer(stuff->oy, byteswanted+4096);
	if (buf == NULL) {
		fprintf(stderr, "error: ogg sync returned no buffer\n");
		return MNG_FALSE;
	}
	/* read the ogg bitstream from the file */
	/* we'll always be short because of the framing overhead  */
	streambytes = fread(buf, 1, byteswanted+4096, stuff->file);
	ogg_sync_wrote(stuff->oy, streambytes);
	/* the ogg layer now has the data and we don't have to worry
	   about it again */
	fprintf(stderr, "read %d bytes from '%s'\n",
		streambytes, stuff->filename);

	fprintf(stderr, "looking for ogg pages...\n");

	/* process any pages we found */
	while (rc = ogg_sync_pageout(stuff->oy, og) > 0) {
		fprintf(stderr, " got page %x:%ld\n",
			ogg_page_serialno(og), ogg_page_pageno(og));
		/* have we seen this (or any) substream before? */
		if (stuff->os == NULL) {
			stuff->os = (ogg_stream_state*)malloc(sizeof(ogg_stream_state));
			if (stuff->os == NULL) {
				fprintf(stderr, "error: couldn't allocate stream state\n");
				return MNG_FALSE;
			}
			ogg_stream_init(stuff->os, ogg_page_serialno(og));
			fprintf(stderr, "creating stream state for logical bitstream %x\n", stuff->os->serialno);
		}
		if (ogg_page_serialno(og) == stuff->os->serialno) {
	/* FIXME: for now, we just assume the substream with the first
	   header page is the mng we want to see */   

		/* packetize it */
		ogg_stream_pagein(stuff->os, og);
		while (ogg_stream_packetout(stuff->os, op) > 0) {
			fprintf(stderr, "  got packet %ld\n", op->packetno);
			/* check for overflow -- should never happen */
			if (*bytesread >= byteswanted) {
				fprintf(stderr, "error: packet data bigger than we thought!\n");
 				return MNG_FALSE;
			}
			copybytes = MIN(op->bytes, byteswanted - *bytesread);

			/* copy it into the mng decode buffer */
			memcpy(buffer + (*bytesread), op->packet, copybytes);
			*bytesread += copybytes;

			fprintf(stderr, "  submitting %d bytes to the mng decoder\n", copybytes);

			/* are we done? */
			if (*bytesread >= byteswanted) {
				/* save any state */
				if (copybytes < op->bytes) {
					stuff->og_last = og;
					stuff->op_last = op;
					stuff->op_bytes = copybytes;
					fprintf(stderr, "saving state: %d bytes left in packet number %ld\n", op->bytes-copybytes, op->packetno);
				} else {
					stuff->og_last = NULL;
					stuff->op_last = NULL;
					stuff->op_bytes= 0;
					fprintf(stderr, " end of page %ld\n", ogg_page_pageno(og));
					free(op);
					free(og);
				}
				return MNG_TRUE;
			}
		}

		} else {
			fprintf(stderr, "dropping page which does not match our serial number\n");
		}
	}
	fprintf (stderr, "no (more) pages: %d\n", rc);

	return MNG_TRUE;
}

/* the header's been read. set up the display stuff_t */
mng_bool mymngprocessheader(mng_handle mng,
		mng_uint32 width, mng_uint32 height)
{
	stuff_t		*stuff;
	SDL_Surface	*screen;
	char		title[256];

//	fprintf(stderr, "our mng is %dx%d\n", width,height);

	screen = SDL_SetVideoMode(width,height, 32, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr, "unable to allocate %dx%d video memory: %s\n", 
			width, height, SDL_GetError());
		return MNG_FALSE;
	}

	/* save the surface pointer */
 	stuff = (stuff_t*)mng_get_userdata(mng);
	stuff->surface = screen;

	/* set a descriptive window title */
	snprintf(title, 256, "mngplay: %s", stuff->filename);
	SDL_WM_SetCaption(title, "mngplay");

	/* in necessary, lock the drawing surface to the decoder
	   can safely fill it. We'll unlock elsewhere before display */
	if (SDL_MUSTLOCK(stuff->surface)) {
		if ( SDL_LockSurface(stuff->surface) < 0 ) {
			fprintf(stderr, "could not lock display surface\n");
			exit(1);
		}
	}

	/* tell the mng decoder about our bit-depth choice */
	/* FIXME: this works on intel. is it correct in general? */
	mng_set_canvasstyle(mng, MNG_CANVAS_BGRA8);

	return MNG_TRUE;
}

/* return a row pointer for the decoder to fill */
mng_ptr mymnggetcanvasline(mng_handle mng, mng_uint32 line)
{
	stuff_t	*stuff;
	SDL_Surface	*surface;
	mng_ptr		row;

	/* dereference our structure */
	stuff = (stuff_t*)mng_get_userdata(mng);

	/* we assume any necessary locking has happened 
	   outside, in the frame level code */
	row = stuff->surface->pixels + stuff->surface->pitch*line;

//	fprintf(stderr, "   returning pointer to line %d (%p)\n", line, row);
 
	return (row);	
}

/* timer */
mng_uint32 mymnggetticks(mng_handle mng)
{
	mng_uint32 ticks;

	ticks = (mng_uint32)SDL_GetTicks();
//	fprintf(stderr, "  %d\t(returning tick count)\n",ticks);

	return(ticks);
}

mng_bool mymngrefresh(mng_handle mng, mng_uint32 x, mng_uint32 y,
			mng_uint32 w, mng_uint32 h)
{
	stuff_t	*stuff;
	SDL_Rect	frame;

	frame.x = x;
	frame.y = y;
	frame.w = w;
	frame.h = h;

	/* dereference our structure */
        stuff = (stuff_t*)mng_get_userdata(mng);

	/* if necessary, unlock the display */
	if (SDL_MUSTLOCK(stuff->surface)) {
                SDL_UnlockSurface(stuff->surface);
        }

        /* refresh the screen with the new frame */
        SDL_UpdateRects(stuff->surface, 1, &frame);
	
	/* in necessary, relock the drawing surface */
        if (SDL_MUSTLOCK(stuff->surface)) {
                if ( SDL_LockSurface(stuff->surface) < 0 ) {
                        fprintf(stderr, "could not lock display surface\n");
                        return MNG_FALSE;
                }
        }
	

	return MNG_TRUE;
}

/* interframe delay callback */
mng_bool mymngsettimer(mng_handle mng, mng_uint32 msecs)
{
	stuff_t	*stuff;

//	fprintf(stderr,"  pausing for %d ms\n", msecs);
	
	/* look up our stream struct */
        stuff = (stuff_t*)mng_get_userdata(mng);

	/* set the timer for when the decoder wants to be woken */
	stuff->delay = msecs;

	return MNG_TRUE;
	
}

mng_bool mymngerror(mng_handle mng, mng_int32 code, mng_int8 severity,
	mng_chunkid chunktype, mng_uint32 chunkseq,
	mng_int32 extra1, mng_int32 extra2, mng_pchar text)
{
	stuff_t	*stuff;
	char		chunk[5];
	
        /* dereference our data so we can get the filename */
        stuff = (stuff_t*)mng_get_userdata(mng);

	/* pull out the chuck type as a string */
	// FIXME: does this assume unsigned char?
	chunk[0] = (char)((chunktype >> 24) & 0xFF);
	chunk[1] = (char)((chunktype >> 16) & 0xFF);
	chunk[2] = (char)((chunktype >>  8) & 0xFF);
	chunk[3] = (char)((chunktype      ) & 0xFF);
	chunk[4] = '\0';

	/* output the error */
	fprintf(stderr, "error playing '%s' chunk %s (%d):\n",
		stuff->filename, chunk, chunkseq);
	fprintf(stderr, "%s\n", text);

	return (0);
}

int mymngquit(mng_handle mng)
{
	stuff_t	*stuff;

	/* dereference our data so we can free it */
	stuff = (stuff_t*)mng_get_userdata(mng);

	/* cleanup. this will call mymngclosestream */
        mng_cleanup(&mng);

	/* free our data */
	free(stuff);
	
	/* quit */
	exit(0);
}

int checkevents(mng_handle mng)
{
	SDL_Event	event;

	/* check if there's an event pending */
	if (!SDL_PollEvent(&event)) {
		return 0;	/* no events pending */
	}

	/* we have an event; process it */
	switch (event.type) {
		case SDL_QUIT:
			mymngquit(mng);	/* quit */ 
			break;
		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
				case SDLK_q:
					mymngquit(mng);
					break;
			}
		default:
			return 1;
	}
}

int main(int argc, char *argv[])
{
	stuff_t	*stuff;
	mng_handle	mng;
	SDL_Rect	updaterect;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <mngfile>\n", argv[0]);
		exit(1);
	}

	/* allocate our stream data structure */
	stuff = (stuff_t*)calloc(1, sizeof(*stuff));
	if (stuff == NULL) {
		fprintf(stderr, "could not allocate stream structure.\n");
		exit(0);
	}

	/* pass the name of the file we want to play */
	stuff->filename = argv[1];

	/* set up the mng decoder for our stream */
        mng = mng_initialize(stuff, mymngalloc, mymngfree, MNG_NULL);
        if (mng == MNG_NULL) {
                fprintf(stderr, "could not initialize libmng.\n");
                exit(1);
        }

        /* set the callbacks */
	mng_setcb_errorproc(mng, mymngerror);
        mng_setcb_openstream(mng, mymngopenstream);
        mng_setcb_closestream(mng, mymngclosestream);
        mng_setcb_readdata(mng, mymngreadstream);
	mng_setcb_gettickcount(mng, mymnggetticks);
	mng_setcb_settimer(mng, mymngsettimer);
	mng_setcb_processheader(mng, mymngprocessheader);
	mng_setcb_getcanvasline(mng, mymnggetcanvasline);
	mng_setcb_refresh(mng, mymngrefresh);
	/* FIXME: should check for errors here */

	/* initialize SDL */
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                fprintf(stderr, "%s: Unable to initialize SDL (%s)\n",
                        argv[0], SDL_GetError());
                exit(1);
        }
	/* arrange to call the shutdown routine before we exit */
        atexit(SDL_Quit);

	/* restrict event handling to the relevant bits */
	SDL_EventState(SDL_KEYDOWN, SDL_IGNORE); /* keyup only */
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

//	fprintf(stderr, "playing mng...maybe.\n");

	mng_readdisplay(mng);

	/* loop though the frames */
	while (stuff->delay) {
//		fprintf(stderr, "  waiting for %d ms\n", stuff->delay);
		SDL_Delay(stuff->delay);

		/* reset the delay in case the decoder
		   doesn't update it again */
		stuff->delay = 0;

		mng_display_resume(mng);

		/* check for user input (just quit at this point) */
		checkevents(mng);
	}

	/* ¿hay alguno? pause before quitting */
	fprintf(stderr, "pausing before shutdown...\n");
	SDL_Delay(3000);

	/* cleanup and quit */
	mymngquit(mng);
}
