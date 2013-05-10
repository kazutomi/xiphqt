/*
	mngplay

	Copyright (C) 2001-2006 Ralph Giles <giles@xiph.org>

	This program my be redistributed under the terms of the
	GNU General Public Licence, version 2, or at your preference,
	any later version.

	this is an SDL based mng player. the code is very rough;
	patches welcome.

	$Id: 2003/12/07 09:45:16 $

	GRR 20010708:  added SDL/libmng/zlib/libjpeg version info, mouse-click
			handling (alternate quit mode); improved automake setup

	Raphael Assenat <raph :at: raphnet.net>
	2003/11/26:    added command line options to run in alternate color depths.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgen.h> // basename

#include <SDL.h>
#include <libmng.h>
#include <ogg/ogg.h>

#define MNG_SIGNATURE "\212MNG\r\n\032\n"

#define DEFAULT_SDL_VIDEO_DEPTH 32

#define OGG_MNG_BUFFER_SIZE 4096

/* structure for keeping track of our mng stream inside the callbacks */
typedef struct {
	FILE		*file;	   /* pointer to the file we're decoding */
	char		*filename; /* pointer to the file's path/name */
	SDL_Surface	*surface;  /* SDL display */
	mng_uint32	delay;     /* ticks to wait before resuming decode */
	int		sdl_video_depth;  /* The depth for SDL_SetVideoMode */
	int		is_ogg;	   /* whether we're parsing an Ogg file */
	ogg_sync_state  ogg_sync;  /* libogg parse structures */
	ogg_stream_state ogg_stream;
} mngstuff;

/* callbacks for the mng decoder */

/* memory allocation; data must be zeroed */
mng_ptr mymngalloc(mng_size_t size)
{
	return (mng_ptr)calloc(1, size);
}

/* memory deallocation */
void mymngfree(mng_ptr p, mng_size_t size)
{
	free(p);
	return;
}

/* pull data from the file into our ogg parser */
int bufferoggdata(mng_handle mng)
{
	mngstuff	*mymng;
	char		*buffer;
	int		bytes;

	/* look up our stream struct */
        mymng = (mngstuff*)mng_get_userdata(mng);

	buffer = ogg_sync_buffer(&mymng->ogg_sync, OGG_MNG_BUFFER_SIZE);
	bytes = fread(buffer, 1, OGG_MNG_BUFFER_SIZE, mymng->file);
	ogg_sync_wrote(&mymng->ogg_sync, bytes);

	return bytes;
}

mng_bool mymngopenstream(mng_handle mng)
{
	mngstuff	*mymng;
	unsigned char   magic[8];
	ogg_page	page;
	int bytes;

	/* look up our stream struct */
        mymng = (mngstuff*)mng_get_userdata(mng);
	
	/* open the file */
	mymng->file = fopen(mymng->filename, "rb");
	if (mymng->file == NULL) {
		fprintf(stderr, "unable to open '%s'\n", mymng->filename);
		return MNG_FALSE;
	}

	/* determine file type */
	mymng->is_ogg = MNG_FALSE;
	bytes = fread(magic, 1, 8, mymng->file);
	if (bytes != 8) {
		fprintf(stderr, "unable to determine file type for '%s'\n", mymng->filename);
		fclose(mymng->file);
		return MNG_FALSE;
	}
	/* return to the start */
	fseek(mymng->file, 0, SEEK_SET);
	if (!memcmp(magic, MNG_SIGNATURE, 8)) {
		/* We have a normal MNG file; return */
		fprintf(stderr, "opening normal MNG stream\n");
		return MNG_TRUE;
	} else if (!memcmp(magic, "OggS", 4)) {

	fprintf(stderr, "opening OggMNG stream\n");
	mymng->is_ogg = MNG_TRUE;

	/* set up the Ogg MNG decoder */
	ogg_sync_init(&mymng->ogg_sync);
	bufferoggdata(mng);
	while (ogg_sync_pageout(&mymng->ogg_sync, &page)>0) {
		ogg_stream_state test;
		ogg_packet packet;

		/* stop after the initial bos pages */
		if (!ogg_page_bos(&page)) break;

		/* set up a parser for each bos page we see */
		ogg_stream_init(&test, ogg_page_serialno(&page));
		ogg_stream_pagein(&test,&page);
		ogg_stream_packetout(&test, &packet);
		if (packet.bytes >8 &&
		    !memcmp(packet.packet, MNG_SIGNATURE, 8)) {
			/* we have a MNG stream; save state and return */
			memcpy(&mymng->ogg_stream, &test, sizeof(test));
			/* tell the library to ignore missing CRCs */
			mng_set_crcmode(mng, 0);
			fprintf(stderr, "Found MNG substream with serialno 0x%08x\n", ogg_page_serialno(&page));
			fprintf(stderr, "Pushing MNG signature\n");
			mng_read_pushsig(mng, mng_it_mng);
			/* the first packet is special, with the 8 byte
			   signature prepended. */
			fprintf(stderr, "returning chunk %c%c%c%c"
				" %ld bytes\n",
				packet.packet[8], packet.packet[9],
				packet.packet[10], packet.packet[11],
				packet.bytes - 4 - 8);
			mng_read_pushchunk(mng,
				packet.packet+8, packet.bytes-8,
				MNG_FALSE);
			return MNG_TRUE;
		}
		/* free scratch parser */
		ogg_stream_clear(&test);
	}

	/* no MNG streams in this Ogg file */
	fprintf(stderr, "Couldn't find a MNG stream in Ogg file '%s'\n",
		mymng->filename);
	return MNG_FALSE;
	}

	/* Unknown file magic, try to parse as a raw MNG */
	return MNG_TRUE;
}

mng_bool mymngclosestream(mng_handle mng)
{
	mngstuff	*mymng;

	/* look up our stream struct */
        mymng = (mngstuff*)mng_get_userdata(mng);

	/* ogg stuff */
	if (mymng->is_ogg) {
		ogg_stream_clear(&mymng->ogg_stream);
		ogg_sync_clear(&mymng->ogg_sync);
	}

	/* close the file */
	fclose(mymng->file);
	mymng->file = NULL;	/* for safety */

	return MNG_TRUE;
}

/* feed data to the decoder */
mng_bool mymngreadstream(mng_handle mng, mng_ptr buffer,
		mng_uint32 size, mng_uint32 *bytesread)
{
	mngstuff *mymng;

	/* look up our stream struct */
	mymng = (mngstuff*)mng_get_userdata(mng);

	/* we don't do anything on OggMNG streams */
	if (mymng->is_ogg) {
		*bytesread = 0;
		//return MNG_FALSE;
		return MNG_TRUE;
	}

	/* read the requested amount of data from the file */
	*bytesread = fread(buffer, 1, size, mymng->file);

	return MNG_TRUE;
}

/* the header's been read. set up the display stuff */
mng_bool mymngprocessheader(mng_handle mng,
		mng_uint32 width, mng_uint32 height)
{
	mngstuff	*mymng;
	SDL_Surface	*screen;
	char		title[256];

//	fprintf(stderr, "our mng is %dx%d\n", width,height);

	/* retreive our user data */
 	mymng = (mngstuff*)mng_get_userdata(mng);
	
	screen = SDL_SetVideoMode(width,height, mymng->sdl_video_depth, SDL_SWSURFACE);
	if (screen == NULL) {
		fprintf(stderr, "unable to allocate %dx%d video memory: %s\n",
			width, height, SDL_GetError());
		return MNG_FALSE;
	}

	printf("SDL Video Mode: %dx%d bpp=%d\n", width, height, mymng->sdl_video_depth);
	
	/* save the surface pointer */
	mymng->surface = screen;

	/* set a descriptive window title */
	snprintf(title, 256, "mngplay: %s", mymng->filename);
	SDL_WM_SetCaption(title, "mngplay");

	/* in necessary, lock the drawing surface so the decoder
	   can safely fill it. We'll unlock elsewhere before display */
	if (SDL_MUSTLOCK(mymng->surface)) {
		if ( SDL_LockSurface(mymng->surface) < 0 ) {
			fprintf(stderr, "could not lock display surface\n");
			exit(1);
		}
	}

/*
	printf("RGBA Masks: %08X %08X %08X %08X\n",
				mymng->surface->format->Rmask,
				mymng->surface->format->Gmask,
				mymng->surface->format->Bmask,
				mymng->surface->format->Amask);
	printf("RGBA Shifts: %08X %08X %08X %08X\n",
				mymng->surface->format->Rshift,
				mymng->surface->format->Gshift,
				mymng->surface->format->Bshift,
				mymng->surface->format->Ashift);
*/
	/* Choose a canvas style which matches the SDL_Surface pixel format */
	switch(mymng->surface->format->BitsPerPixel)
	{
		case 32:
			if (mymng->surface->format->Amask==0) {
				/* No alpha (padding byte) */
				if (mymng->surface->format->Bshift==0) {
					/* Blue first */
					mng_set_canvasstyle(mng, MNG_CANVAS_BGRX8);
				} else {
					/* Red first */
					fprintf(stderr, "No matching mng canvas for sdl pixel format. Colors may be wrong.\n");
					mng_set_canvasstyle(mng, MNG_CANVAS_BGRX8);
				}
			}
			else {
				/* Real alpha */
				if (mymng->surface->format->Bshift==0) {
					/* Blue first */
					mng_set_canvasstyle(mng, MNG_CANVAS_BGRA8);
				} else {
					/* Red first */
					mng_set_canvasstyle(mng, MNG_CANVAS_RGBA8);
				}
			}
			break;
		case 24:
			if (mymng->surface->format->Amask==0) {
				/* No alpha here should mean true rgb24bit */
				if (mymng->surface->format->Bshift==0) {
					/* Blue first */
					mng_set_canvasstyle(mng, MNG_CANVAS_BGR8);
				} else {
					/* Red first */
					mng_set_canvasstyle(mng, MNG_CANVAS_RGB8);
				}
			}
			else {
				/* If there is an alpha and we are in 24 bpp, this must
				 * mean rgb5658 */
				if (mymng->surface->format->Bshift==0) {
					/* Blue first */
					mng_set_canvasstyle(mng, MNG_CANVAS_BGRA565);
				} else {
					/* Red first */
					mng_set_canvasstyle(mng, MNG_CANVAS_RGBA565);
				}
			}
			break;
		case 16:
			if (mymng->surface->format->Bshift==0) {
				/* Blue first */
				mng_set_canvasstyle(mng, MNG_CANVAS_BGR565);
			} else {
				/* Red first */
				mng_set_canvasstyle(mng, MNG_CANVAS_RGB565);
			}			
			break;
		default:
			return MNG_FALSE;
	}

	return MNG_TRUE;
}

/* return a row pointer for the decoder to fill */
mng_ptr mymnggetcanvasline(mng_handle mng, mng_uint32 line)
{
	mngstuff	*mymng;
	mng_ptr		row;

	/* dereference our structure */
	mymng = (mngstuff*)mng_get_userdata(mng);

	/* we assume any necessary locking has happened
	   outside, in the frame level code */
	row = mymng->surface->pixels + mymng->surface->pitch*line;

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
	mngstuff	*mymng;
	SDL_Rect	frame;

	frame.x = x;
	frame.y = y;
	frame.w = w;
	frame.h = h;

	/* dereference our structure */
        mymng = (mngstuff*)mng_get_userdata(mng);

	/* if necessary, unlock the display */
	if (SDL_MUSTLOCK(mymng->surface)) {
                SDL_UnlockSurface(mymng->surface);
        }

        /* refresh the screen with the new frame */
        SDL_UpdateRects(mymng->surface, 1, &frame);
	
	/* in necessary, relock the drawing surface */
        if (SDL_MUSTLOCK(mymng->surface)) {
                if ( SDL_LockSurface(mymng->surface) < 0 ) {
                        fprintf(stderr, "could not lock display surface\n");
                        return MNG_FALSE;
                }
        }
	

	return MNG_TRUE;
}

/* interframe delay callback */
mng_bool mymngsettimer(mng_handle mng, mng_uint32 msecs)
{
	mngstuff	*mymng;

//	fprintf(stderr,"  pausing for %d ms\n", msecs);
	
	/* look up our stream struct */
        mymng = (mngstuff*)mng_get_userdata(mng);

	/* set the timer for when the decoder wants to be woken */
	mymng->delay = msecs;

	return MNG_TRUE;
	
}

mng_bool mymngerror(mng_handle mng, mng_int32 code, mng_int8 severity,
	mng_chunkid chunktype, mng_uint32 chunkseq,
	mng_int32 extra1, mng_int32 extra2, mng_pchar text)
{
	mngstuff	*mymng;
	char		chunk[5];
	
        /* dereference our data so we can get the filename */
        mymng = (mngstuff*)mng_get_userdata(mng);

	/* pull out the chuck type as a string */
	// FIXME: does this assume unsigned char?
	chunk[0] = (char)((chunktype >> 24) & 0xFF);
	chunk[1] = (char)((chunktype >> 16) & 0xFF);
	chunk[2] = (char)((chunktype >>  8) & 0xFF);
	chunk[3] = (char)((chunktype      ) & 0xFF);
	chunk[4] = '\0';

	/* output the error */
	fprintf(stderr, "error playing '%s' chunk %s (%d):\n",
		mymng->filename, chunk, chunkseq);
	fprintf(stderr, "%s\n", text);

	return (0);
}

int mymngquit(mng_handle mng)
{
	mngstuff	*mymng;

	/* dereference our data so we can free it */
	mymng = (mngstuff*)mng_get_userdata(mng);

	/* cleanup. this will call mymngclosestream */
        mng_cleanup(&mng);

	/* free our data */
	free(mymng);
	
	/* quit */
	exit(0);
}

int pullchunks(mng_handle mng)
{
	mngstuff	*mymng;
	int		bytes;
	ogg_page	page;
	ogg_packet	packet;

        /* dereference our user data */
        mymng = (mngstuff*)mng_get_userdata(mng);

	/* make sure we're an OggMNG stream */
	if (!mymng->is_ogg) return MNG_FALSE;

	/* pull in more data */
	bytes = bufferoggdata(mng);
	if (bytes <= 0) return MNG_FALSE;

	/* pull any new pages from the stream */
	while (ogg_sync_pageout(&mymng->ogg_sync, &page)>0) {
		/* we don't need the check the serialno,
		   ogg_stream_pagein will discard the page
		   if it does not match */
		ogg_stream_pagein(&mymng->ogg_stream, &page);
		fprintf(stderr, "got new page no %ld\n", mymng->ogg_stream.pageno);
		/* pull out any new packets */
		while (ogg_stream_packetout(&mymng->ogg_stream, &packet)>0) {
			/* each OggMNG packet is a chunk, minus
			   length and CRC fields */
			fprintf(stderr, "  returning chunk %c%c%c%c"
				" %ld bytes\n",
				packet.packet[0], packet.packet[1],
				packet.packet[2], packet.packet[3],
				packet.bytes);
			mng_read_pushchunk(mng,
					packet.packet, packet.bytes,
					MNG_FALSE);
		}
	}

	return MNG_TRUE;
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
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			mymngquit(mng);
			break;
		case SDL_KEYUP:
			switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
				case SDLK_q:
					mymngquit(mng);
					break;
				default:
					/* ignore unknown keys */
					break;
			}
			/* FALL THROUGH */
		default:
			return 1;
	}

	return 0;   /* GRR ADDED:  non-void function */
}

int main(int argc, char *argv[])
{
	mngstuff	*mymng;
	mng_handle	mng;
        mng_retcode     ret;
	int		done = 0;

	if (argc < 2) {
		const SDL_version *pSDLver = SDL_Linked_Version();

		fprintf(stderr, "Usage:  %s mngfile [depth]\n\n", basename(argv[0]));
		fprintf(stderr, "        where 'depth' is 15,16,24 or 32\n");
		fprintf(stderr,
			"  Compiled with SDL %d.%d.%d; using SDL %d.%d.%d.\n",
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
			pSDLver->major, pSDLver->minor, pSDLver->patch);
		fprintf(stderr, "  Compiled with libmng %s; using libmng %s.\n",
			MNG_VERSION_TEXT, mng_version_text());
		fprintf(stderr, "  Compiled with zlib %s; using zlib %s.\n",
			ZLIB_VERSION, zlib_version);
#ifdef JPEG_LIB_VERSION
		{
			int major = JPEG_LIB_VERSION / 10;
			int minor = JPEG_LIB_VERSION % 10;
			char minoralpha[2];

			if (minor) {
				minoralpha[0] = (char)(minor - 1 + 'a');
				minoralpha[1] = '\0';
			} else
				minoralpha[0] = '\0';
			fprintf(stderr, "  Compiled with libjpeg %d%s.\n",
				major, minoralpha);
		}
#endif
		fprintf(stderr,
			"\nPress Esc or Q, or click mouse button, to quit.\n");
		exit(1);
	}

	/* allocate our stream data structure */
	mymng = (mngstuff*)calloc(1, sizeof(*mymng));
	if (mymng == NULL) {
		fprintf(stderr, "could not allocate stream structure.\n");
		exit(0);
	}

	/* pass the name of the file we want to play */
	mymng->filename = argv[1];

	/* pass the color depth we wish to use */
	if (argc>=3) {
		mymng->sdl_video_depth = atoi(argv[2]);
		switch(mymng->sdl_video_depth) {
			case 15:
			case 16:
			case 24:
			case 32:
				break;
			default:
				fprintf(stderr, "Unsupported color depth. Choices are: 15, 16, 24 and 32\n");
				exit(1);
		}
	}
	else {
		mymng->sdl_video_depth = DEFAULT_SDL_VIDEO_DEPTH;
	}
	
	/* set up the mng decoder for our stream */
        mng = mng_initialize(mymng, mymngalloc, mymngfree, MNG_NULL);
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
//	SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
//	SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

//	fprintf(stderr, "playing mng...maybe.\n");

	ret = mng_readdisplay(mng);
//	fprintf(stderr, "mng_readdisplay() returned %d\n", ret);
	if (ret == MNG_NEEDMOREDATA && mymng->is_ogg) {
		fprintf(stderr, "pulling more chunks prior to main loop...\n");
		pullchunks(mng);
	}

	/* loop though the frames */
	while (!done) {
//		fprintf(stderr, "  waiting for %d ms\n", mymng->delay);
		SDL_Delay(mymng->delay);

		/* reset the delay in case the decoder
		   doesn't update it again */
		mymng->delay = 0;

		ret = mng_display_resume(mng);
//		fprintf(stderr, "mng_readdisplay() returned %d\n", ret);
		if (mymng->is_ogg && (ret == MNG_NEEDMOREDATA ||
				ret == MNG_NEEDTIMERWAIT)) {
			/* FIXME: heisenbug clobbers the file pointer */
			if (mymng->file) pullchunks(mng);
		}

		if (ret == 0) done = 1;

		/* check for user input (just quit at this point) */
		checkevents(mng);
	}

	/* ¿hay alguno? pause before quitting */
	fprintf(stderr, "pausing before shutdown...\n");
	SDL_Delay(1000);

	/* cleanup and quit */
	mymngquit(mng);

	return 0;
}
