/********************************************************************
 *                                                                  *
 * COPYRIGHT (C) 2004 Xiph.Org Foundation http://www.xiph.org/      *
 *                                                                  *
 * Distributed under the terms of the GNU GPL                       *
 *                                                                  *
 ********************************************************************/

/* this is a liboggz based player for the Xiph.org codecs
   encapsulated in Ogg bitstreams. It might want to eventually
   replace ogg123.

   quick-and-dirty for now. compile with:

	gcc -o theoraplay theoraplay.c \
	`sdl-config --cflags` `pkg-config --cflags theora vorbis oggz ogg` \
	`sdl-config --libs` `pkg-config --libs theora vorbis oggz ogg`

   relies only on SDL for a/v playback.
*/

#include <stdio.h>
#include <stdlib.h>

#include <oggz/oggz.h>
#include <theora/theora.h>
#include <vorbis/codec.h>

#include <SDL.h>

static int read_packet_null(OGGZ *oggz, ogg_packet *op, long serialno,
	void *user_data)
{
	/* report end of stream */
	if (op->e_o_s) {
		fprintf(stderr, "0x%08x end of stream\n");
	}
	/* otherwise do nothing */
	return 0;
}

typedef struct {
	int decoding, flag;
	theora_info ti;
	theora_comment tc;
	theora_state td;
	SDL_Surface *screen;
	SDL_Overlay *yuv_overlay;
	SDL_Rect rect;
} theora_playback;

static int video_open(theora_playback *this)
{
	theora_info *ti = &(this->ti);

	fprintf(stderr, "Opening %dx%d video window\n",
		ti->frame_width, ti->frame_height);

	this->screen =  
		SDL_SetVideoMode(ti->frame_width, ti->frame_height,
		 0, SDL_SWSURFACE);
  	if (this->screen == NULL ) {
    		fprintf(stderr, "Unable to set %dx%d video: %s\n",
            		ti->frame_width,ti->frame_height,
			SDL_GetError());
    		return 1;
  	}

	this->yuv_overlay = 
		SDL_CreateYUVOverlay(ti->frame_width, ti->frame_height,
                                     SDL_YV12_OVERLAY,
                                     this->screen);
	if (this->yuv_overlay == NULL) {
		fprintf(stderr, "SDL: Couldn't create SDL_yuv_overlay: %s\n",
            		SDL_GetError());
    		return 2;
	}
    
	this->rect.x = 0;
  	this->rect.y = 0;
  	this->rect.w = ti->frame_width;
  	this->rect.h = ti->frame_height;

	SDL_DisplayYUVOverlay(this->yuv_overlay, &(this->rect));


}

static void video_write(theora_playback *this)
{
	SDL_Overlay *overlay = this->yuv_overlay;
	theora_info *ti = &(this->ti);
	yuv_buffer yuv;
	int crop_offset;
	int i;
	theora_decode_YUVout(&(this->td),&yuv);
#if DEBUG
	fprintf(stderr, "playing video frame at %ld\n", 
		this->td.granulepos);
#endif
	if (SDL_LockYUVOverlay(overlay) < 0) return;

  crop_offset=ti->offset_x+yuv.y_stride*ti->offset_y;
  for(i=0;i<overlay->h;i++)
    memcpy(overlay->pixels[0]+overlay->pitches[0]*i,
           yuv.y+crop_offset+yuv.y_stride*i,
           overlay->w);
  crop_offset=(ti->offset_x/2)+(yuv.uv_stride)*(ti->offset_y/2);
  for(i=0;i<overlay->h/2;i++){
    memcpy(overlay->pixels[1]+overlay->pitches[1]*i,
           yuv.v+crop_offset+yuv.uv_stride*i,
           overlay->w/2);
    memcpy(overlay->pixels[2]+overlay->pitches[2]*i,
           yuv.u+crop_offset+yuv.uv_stride*i,
           overlay->w/2);
  }

	SDL_UnlockYUVOverlay(this->yuv_overlay);
	
	SDL_DisplayYUVOverlay(this->yuv_overlay, &(this->rect));
}

static int read_packet_theora(OGGZ *oggz, ogg_packet *op, long serialno,
	void *user_data)
{
	theora_playback *this = (theora_playback*)user_data;
	theora_state *td = &(this->td);
	int result = 0;

	/* are we still looking for header packets? */	
	if (this->flag < 3) {
		result = theora_decode_header(&(this->ti),&(this->tc),op);
		if (result < 0) {
			fprintf(stderr, "0x%08x error parsing theora header!\n", serialno);
			return result;
		}
		this->flag++;
		if (this->flag == 1) {
			fprintf(stderr, "0x%08x Theora Video: %dx%d frames at %.3fHz\n",
				serialno, 
				this->ti.frame_width, this->ti.frame_height,
				(float)this->ti.fps_numerator/(float)this->ti.fps_denominator);
			fprintf(stderr, "0x%08x Theora Video: pixel aspect %d:%d\n",
				serialno, 
				this->ti.aspect_numerator,
				this->ti.aspect_denominator);

		}
	} else {
		if (this->flag < 4) {
			theora_decode_init(td,&(this->ti));
			video_open(this);			
			this->flag++;
#ifdef DEBUG
			fprintf(stderr, "0x%08x encoded by: %s\n", 
				serialno, this->tc.vendor);
#endif
		}
		theora_decode_packetin(td,op);
		video_write(this);
	}
	return 0;
}

typedef struct {
	int decoding, flag;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
} vorbis_playback;

static int read_packet_vorbis(OGGZ *oggz, ogg_packet *op, long serialno,
	void *user_data)
{
	vorbis_playback *this = (vorbis_playback*)user_data;
	int result = 0;

	/* are we still looking for header packets? */	
	if (this->flag < 3) {
		result = vorbis_synthesis_headerin(&(this->vi),&(this->vc),op);
		if (result < 0) {
			fprintf(stderr, "0x%08x error parsing vorbis header!\n", serialno);
			return result;
		}
		this->flag++;
		if (this->flag == 1) {
			fprintf(stderr, "0x%08x Vorbis Audio: %d channels at %dHz\n",
				serialno, this->vi.channels, this->vi.rate);
		}
	} else {
		if (this->flag < 4) {
			vorbis_synthesis_init(&(this->vd),&(this->vi));
			vorbis_block_init(&(this->vd),&(this->vb));
			// audio_open(this);
			this->flag++;
#if DEBUG
			fprintf(stderr, "0x%08x encoded by: %s\n", 
				serialno, this->vc.vendor);
#endif
		}
		if (vorbis_synthesis(&(this->vb),op) == 0)
			vorbis_synthesis_blockin(&(this->vd),&(this->vb));
		// audio_write(this);
	}
	return 0;
}


static int read_packet_dispatch(OGGZ *oggz, ogg_packet *op, long serialno,
	void *user_data)
{
	const unsigned char theora_magic[] = "\x80theora";
	const unsigned char vorbis_magic[] = "\x01vorbis";

	/* we're called for new streams; try and identify the type */
	if (!memcmp(op->packet, theora_magic, 7)) {
		theora_playback *pb = malloc(sizeof(*pb));
		pb->flag = 0; pb->decoding = 1;
		theora_info_init(&(pb->ti));
		theora_comment_init(&(pb->tc));
		if (!read_packet_theora(oggz, op, serialno, pb))
			oggz_set_read_callback(oggz, serialno, read_packet_theora, pb);
	} else if (!memcmp(op->packet, vorbis_magic, 7)) {
		vorbis_playback *pb = malloc(sizeof(*pb));
		pb->flag = 0; pb->decoding = 1;
		vorbis_info_init(&(pb->vi));
		vorbis_comment_init(&(pb->vc));
		if (!read_packet_vorbis(oggz, op, serialno, pb))
			oggz_set_read_callback(oggz, serialno, read_packet_vorbis, pb);
	} else {
		fprintf(stderr, "0x%08x unrecognized codec!\n", serialno);
		oggz_set_read_callback(oggz, serialno, read_packet_null, NULL);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	OGGZ *oggz;

	if (argc < 2) {
		printf("usage: %s <file.ogg>\n", argv[0]);
	}

	oggz = oggz_open((char*)argv[1], OGGZ_READ | OGGZ_AUTO);
	if (oggz == NULL) {
		fprintf(stderr, "unable to open '%s'\n", argv[1]);
		exit(1);
	}

	/* install our dispatcher for new streams */
	oggz_set_read_callback(oggz, -1, read_packet_dispatch, NULL);

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Unable to initialize playback: %s\n",
			SDL_GetError());
		return 1;
	}

	while (oggz_read(oggz, 4096) > 0);

	oggz_close(oggz);

	SDL_Quit();

	return 0;
}
