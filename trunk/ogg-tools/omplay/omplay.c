/*****************************************************************

    This file is part of omplay.

    omplay is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    omplay is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with omplay; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    This code is copyright (c) 2001 by the Xiph.org Foundation 
    (http://www.xiph.org/).
**********************************************************************/

/* omplay.c
**
** main program for omplay, the Ogg MIDI player
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <alsa/asoundlib.h>

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "midi.h"

#define STATUS_FILE_PLAY "status.PLAY"
#define STATUS_FILE_PAUSE "status.PAUSE"
#define STATUS_FILE_FFWD "status.FFWD"
#define FFWD_SPEED 4

typedef struct status_file_tag
{
	int fd;
	char *filemap;
	size_t len;
	int status;
} status_file_t;

typedef struct device_tag
{
	// settings
	char *pcm_device;
	snd_seq_addr_t seq_addr;

	// device handles
	snd_pcm_t *pcmhandle;
	snd_seq_t *seqhandle;
	// audio device stuff
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_info_t *pcminfo;
	snd_pcm_status_t *pcmstat;
	int card;
	int device;
	int subdevice;
	// midi device stuff
	int q;
	int srcport;

	FILE *fp;
	ogg_sync_state oy;
	ogg_stream_state *os_v;
	ogg_stream_state *os_m;

	int midi_serialno;
	int vorbis_serialno;

	midi_info_t mi;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;

	// status files
	status_file_t play;
	status_file_t pause;
	status_file_t ffwd;

	int fast_forwarding;
	int verbose;
} device_t;

device_t device;

int open_devices(void)
{
	int err;
	
	err = snd_pcm_open(&device.pcmhandle, device.pcm_device, SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) return err;
	err = snd_seq_open(&device.seqhandle, "hw", SND_SEQ_OPEN_DUPLEX, 0);
	if (err < 0) {
		snd_pcm_close(device.pcmhandle);
		return err;
	}

	return err;
}

void close_devices(void)
{
	snd_seq_free_queue(device.seqhandle, device.q);
	snd_seq_close(device.seqhandle);
	snd_pcm_close(device.pcmhandle);
}

int configure_audio_device(void)
{
	int err;

	// hardware parameters
	snd_pcm_hw_params_malloc(&device.hwparams);
	err = snd_pcm_hw_params_any(device.pcmhandle, device.hwparams);
	if (err < 0) return err;
	err = snd_pcm_hw_params_set_access(device.pcmhandle, device.hwparams, 
					   SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0) return err;
	err = snd_pcm_hw_params_set_format(device.pcmhandle, device.hwparams, 
					   SND_PCM_FORMAT_S16_LE);
	if (err < 0) return err;
	err = snd_pcm_hw_params_set_channels(device.pcmhandle, device.hwparams, 
					     device.vi.channels);
	if (err < 0) return err;
	err = snd_pcm_hw_params_set_rate_near(device.pcmhandle, device.hwparams,
					      device.vi.rate, 0);
	if (err < 0) return err;
	err = snd_pcm_hw_params_set_period_size(device.pcmhandle, device.hwparams,
						32, 0);
	if (err < 0) return err;
	err = snd_pcm_hw_params_set_periods(device.pcmhandle, device.hwparams,
					    100, 0);
	if (err < 0) return err;
	err = snd_pcm_hw_params(device.pcmhandle, device.hwparams);
	if (err < 0) return err;

	return 0;

	// software parameters
	snd_pcm_sw_params_malloc(&device.swparams);
	err = snd_pcm_sw_params_set_xfer_align(device.pcmhandle, device.swparams, 1);
	if (err < 0) return err;
	err = snd_pcm_sw_params(device.pcmhandle, device.swparams);
	if (err < 0) return err;
	
	return 0;
}

int configure_midi_device(void)
{
	int err;

	device.q = snd_seq_alloc_queue(device.seqhandle);
	err = snd_seq_create_simple_port(device.seqhandle, NULL,
					 SND_SEQ_PORT_CAP_WRITE |
					 SND_SEQ_PORT_CAP_SUBS_WRITE |
					 SND_SEQ_PORT_CAP_READ,
					 SND_SEQ_PORT_TYPE_MIDI_GENERIC);
	if (err < 0) {
		snd_seq_free_queue(device.seqhandle, device.q);
		return err;
	}
	device.srcport = err;
	err = snd_seq_connect_to(device.seqhandle, device.srcport,
				 device.seq_addr.client, device.seq_addr.port);
	if (err < 0) {
		snd_seq_free_queue(device.seqhandle, device.q);
		return err;
	}

	return 0;
}

int set_midi_tempo(void)
{
	int err;
	snd_seq_queue_tempo_t *qtempo;

	snd_seq_queue_tempo_alloca(&qtempo);
	snd_seq_queue_tempo_set_ppq(qtempo, device.mi.ticks);
	snd_seq_queue_tempo_set_tempo(qtempo, device.mi.tempo);
	err = snd_seq_set_queue_tempo(device.seqhandle, device.q, qtempo);
	if (err < 0) {
		printf("Error setting queue tempo\n");
		snd_seq_free_queue(device.seqhandle, device.q);
		return err;
	}

	return 0;
}

int sync_devices(void)
{
	int err;
	snd_seq_queue_timer_t *qt;
	snd_timer_id_t *qtid;

	// get pcm device information
	snd_pcm_info_malloc(&device.pcminfo);
	err = snd_pcm_info(device.pcmhandle, device.pcminfo);
	if (err < 0) return err;
	err = snd_pcm_info_get_card(device.pcminfo);
	if (err < 0) return err;
	device.card = err;
	err = snd_pcm_info_get_device(device.pcminfo);
	if (err < 0) return err;
	device.device = err;
	err = snd_pcm_info_get_subdevice(device.pcminfo);
	if (err < 0) return err;
	device.subdevice = err << 1;

	if (device.verbose >= 1)
		printf("using card %d, device %d, subdev %d\n", device.card, device.device, device.subdevice);

	snd_pcm_status_malloc(&device.pcmstat);

	// setup the queue timer

	snd_seq_queue_timer_alloca(&qt);
	snd_timer_id_alloca(&qtid);

	err = snd_seq_get_queue_timer(device.seqhandle, device.q, qt);
	if (err < 0) {
		return err;
	}
	snd_timer_id_set_class(qtid, SND_TIMER_CLASS_PCM);
	snd_timer_id_set_sclass(qtid, SND_TIMER_SCLASS_NONE);
	snd_timer_id_set_card(qtid, device.card);
	snd_timer_id_set_device(qtid, device.device);
	snd_timer_id_set_subdevice(qtid, device.subdevice);
	snd_seq_queue_timer_set_type(qt, SND_SEQ_TIMER_ALSA);
	snd_seq_queue_timer_set_id(qt, qtid);
	snd_seq_set_queue_timer(device.seqhandle, device.q, qt);
	return 0;
}

void open_file(char *filename)
{
	int numpages = 0;
	ogg_stream_state *temp;
	ogg_page og;
	int err = 0;
	ogg_packet op;
	int bytes;
	int serialno;
	int i;
	char *buffer;
	char buf[1024];

	// open the file
	device.fp = fopen(filename, "r");
	if (device.fp == NULL) {
		printf("Could not open file %s.\n", filename);
		close_devices();
	        exit(1);
	}

	// setup sync and stream states
	ogg_sync_init(&device.oy);
	device.os_m = (ogg_stream_state *)malloc(sizeof(ogg_stream_state));
	device.os_v = (ogg_stream_state *)malloc(sizeof(ogg_stream_state));
	
	// get the headers
	while (!feof(device.fp) && numpages < 2) {
		while (numpages < 2 && (err = ogg_sync_pageout(&device.oy, &og)) == 1) {
			// play stuff here
			if (numpages == 0) {
				ogg_stream_init(device.os_v, ogg_page_serialno(&og));
				device.vorbis_serialno = ogg_page_serialno(&og);
				if (ogg_stream_pagein(device.os_v, &og) < 0) {
					printf("Error reading first page\n");
					close_devices();
					exit(1);
				}
			}
			if (numpages == 1) {
				ogg_stream_init(device.os_m, ogg_page_serialno(&og));
				device.midi_serialno = ogg_page_serialno(&og);
				if (ogg_stream_pagein(device.os_m, &og) < 0) {
					printf("Error reading second page\n");
					close_devices();
					exit(1);
				}
			}

			numpages++;
		}
		
		if (err < 0) break;

		bytes = fread(buf, 1, 1024, device.fp);
		buffer = ogg_sync_buffer(&device.oy, bytes);
		memcpy(buffer, buf, bytes);
		ogg_sync_wrote(&device.oy, bytes);
	}

	if (numpages != 2) {
		printf("Couldn't read headers.\n");
		close_devices();
		exit(1);
	}


	// first try to verify os_v as a vorbis stream.
	vorbis_info_init(&device.vi);
	vorbis_comment_init(&device.vc);
	if (ogg_stream_packetpeek(device.os_v, &op) < 0) {
		printf("Error reading intial header packet\n");
		close_devices();
		exit(1);
	}

	if (vorbis_synthesis_headerin(&device.vi, &device.vc, &op) < 0) {
		// this is not the vorbis stream
		// so swap the stream states
		temp = device.os_v;
		device.os_v = device.os_m;
		device.os_m = temp;

		serialno = device.vorbis_serialno;
		device.vorbis_serialno = device.midi_serialno;
		device.midi_serialno = serialno;

		// and try again
		if (ogg_stream_packetout(device.os_v, &op) < 0) {
			printf("Error reading intial header packet for Vorbis\n");
			close_devices();
			exit(1);
		}

		if (vorbis_synthesis_headerin(&device.vi, &device.vc, &op) < 0) {
			printf("No vorbis stream found.\n");
			close_devices();
			exit(1);
		}
	} else {
		ogg_stream_packetout(device.os_v, &op);
	}

	// at this point we've verified vorbis, and os_m should point to the midi stream
	// but we need to verify it
	if (ogg_stream_packetout(device.os_m, &op) < 0) {
		printf("Error reading intial header packet for MIDI.\n");
		close_devices();
		exit(1);
	}

	midi_header_init(&device.mi);
	midi_header_set_queue(&device.mi, device.q);

	if (midi_headerin(&device.mi, &op) < 0) {
		printf("No MIDI stream found.\n");
		close_devices();
		exit(1);
	}

	// finish reading vorbis headers
	i = 0;
	while (i < 2) {
		while (i < 2) {
			err = ogg_sync_pageout(&device.oy, &og);
			if (err == 0) break; // need more data
			if (err == 1) {
				if (ogg_page_serialno(&og) == device.midi_serialno) {
					// got a midi page
					ogg_stream_pagein(device.os_m, &og);
					break;
				}
				ogg_stream_pagein(device.os_v, &og);
				while (i < 2) {
					err = ogg_stream_packetout(device.os_v, &op);
					if (err == 0) break;
					if (err < 0) {
						printf("Corrupt Vorbis headers.\n");
						close_devices();
						exit(1);
					}
					vorbis_synthesis_headerin(&device.vi, &device.vc, &op);
					i++;
				}
			}
		}
		
		buffer = ogg_sync_buffer(&device.oy, 1024);
		bytes = fread(buffer, 1, 1024, device.fp);
		if (bytes == 0 && i < 2) {
			printf("Premature end of file.\n");
			close_devices();
			exit(1);
		}
		ogg_sync_wrote(&device.oy, bytes);
	}

	vorbis_synthesis_init(&device.vd, &device.vi);
	vorbis_block_init(&device.vd, &device.vb);
}

void close_file(void)
{
	midi_header_clear(&device.mi);

	vorbis_comment_clear(&device.vc);
	vorbis_info_clear(&device.vi);
	ogg_stream_clear(device.os_v);
	ogg_stream_clear(device.os_m);
	free(device.os_v);
	free(device.os_m);
	ogg_sync_clear(&device.oy);

	fclose(device.fp);
}	

void start_queue(void)
{
	snd_seq_event_t ev;


//	snd_seq_ev_set_queue_pos_tick(&ev, device.q, 0);
	snd_seq_ev_clear(&ev);
	snd_seq_ev_schedule_tick(&ev, device.q, 0, 0);
	ev.type = SND_SEQ_EVENT_NOTE;
	ev.data.note.note = 44;
	ev.data.note.velocity = 0;
	ev.data.note.duration = 120;
	ev.source.port = device.srcport;
	ev.dest = device.seq_addr;

	snd_seq_event_output(device.seqhandle, &ev);
	snd_seq_drain_output(device.seqhandle);
}

static int64_t samples_played = 0;

void decode_midi_packet(ogg_packet *op)
{
	snd_seq_event_t ev;
	int err, result;
	snd_seq_queue_status_t *qs;


	snd_seq_queue_status_alloca(&qs);

	// submit packet
	midi_packetin(&device.mi, op);

	while ((err = midi_eventout(&device.mi, &ev)) == 0) {
		snd_pcm_status(device.pcmhandle, device.pcmstat);
		snd_seq_get_queue_status(device.seqhandle, device.q, qs);
		if (ev.type == 0) {
			continue;
		}
		if (ev.type == SND_SEQ_EVENT_TEMPO) {
			if (device.verbose >= 2)
				printf("Changing tempo (us/beat) to: %d\n", ev.data.queue.param.value);
			device.mi.tempo = ev.data.queue.param.value;
			snd_seq_change_queue_tempo(device.seqhandle, device.q, 
						   ev.data.queue.param.value, NULL);
			continue;
		}
		ev.dest = device.seq_addr;
		ev.source.port = device.srcport;
		result = snd_seq_event_output(device.seqhandle, &ev);
	        if (result < 0) printf("Event output result = %d\n", result);
		snd_seq_drain_output(device.seqhandle);
	}

	if (err < 0 && err != EDATA) {
		printf("MIDI packet decoding error (%d).\n", err);
		close_devices();
		exit(1);
	}
}

void decode_vorbis_packet(ogg_packet *op)
{
	float **pcm;
	int samples;
	ogg_int16_t convbuffer[4096];
	int convsize = 4096 / device.vi.channels;
	int i, err;
	unsigned long count, pos;
	snd_seq_queue_status_t *qs;

	snd_seq_queue_status_alloca(&qs);

	if (vorbis_synthesis(&device.vb, op) == 0)
		vorbis_synthesis_blockin(&device.vd, &device.vb);

	while ((samples = vorbis_synthesis_pcmout(&device.vd, &pcm)) > 0) {
		int j;
		int clipflag = 0;
		int bout = (samples < convsize ? samples : convsize);

		for (i = 0; i < device.vi.channels; i++) {
			ogg_int16_t *ptr = convbuffer+i;
			float *mono = pcm[i];

			for (j = 0; j < bout; j++) {
				int val = mono[j] * 32767.f;

				if (val > 32767) {
					val = 32767;
					clipflag = 1;
				}
				if (val < -32768) {
					val = -32768;
					clipflag = 1;
				}

				*ptr = val;
				ptr += device.vi.channels;
			}
		}

		// write data
		if (!device.fast_forwarding) {
			count = bout;
		} else {
			count = bout / FFWD_SPEED;
			for (i = 0; i < count; i++) {
				for (j = 0; j < device.vi.channels; j++) {
					convbuffer[i*device.vi.channels + j] = convbuffer[i*FFWD_SPEED*device.vi.channels + j];
				}
			}
		}
		pos = 0;
		while (1) {
			do {
				err = snd_pcm_writei(device.pcmhandle, &convbuffer[pos], count);
				if (err > 0) {
					if (device.fast_forwarding)
						samples_played += err * FFWD_SPEED;
					else
						samples_played += err;
					count -= err;
					pos += err*2;
				}
			} while (count > 0 && (err > 0 || err == -EAGAIN));
			if (err == -EPIPE) {
				if (device.verbose >= 1)
					printf("audio underrun\n");
				snd_pcm_prepare(device.pcmhandle);
			} else {
				break;
			}
		}
		
		snd_pcm_status(device.pcmhandle, device.pcmstat);
		snd_seq_get_queue_status(device.seqhandle, device.q, qs);
		if (device.verbose >= 3)
			printf("queue is at %d, samples_played == %lld\n", snd_seq_queue_status_get_tick_time(qs), samples_played - snd_pcm_status_get_delay(device.pcmstat));

		vorbis_synthesis_read(&device.vd, bout);
	}
}

ogg_page *copy_page(ogg_page *page)
{
	ogg_page *og;

	og = (ogg_page *)malloc(sizeof(ogg_page));
	og->header_len = page->header_len;
	og->body_len = page->body_len;
	og->header = (char *)malloc(og->header_len);
	og->body = (char *)malloc(og->body_len);
	memcpy(og->header, page->header, og->header_len);
	memcpy(og->body, page->body, og->body_len);

	return og;
}

int init_status_file(status_file_t *sf, char *name, char defval)
{
	/* Create/open the file */
	sf->fd = open(name, O_RDWR | O_CREAT);
	if (sf->fd < 0) return -1;

	/* Map the file to memory */
	sf->filemap = mmap(0, 1, PROT_READ | PROT_WRITE, MAP_SHARED, sf->fd, 0);
	if (sf->filemap < 0) {
		close(sf->fd);
		return -1;
	}

	/* Initialize default value */
	sf->filemap[0] = defval;
	
	/* Flush to file and update other mappings */
	if (msync(sf->filemap, 1, MS_SYNC | MS_INVALIDATE) < 0) {
		munmap(sf->filemap, 1);
		close(sf->fd);
		return -1;
	}

	sf->status = (defval != '0');

	return 0;
}

int setup_status_files(void)
{
	if (init_status_file(&device.play, STATUS_FILE_PLAY, '1') != 0) {
		printf("Could not initialize status file %s\n", STATUS_FILE_PLAY);
		return -1;
	}

	if (init_status_file(&device.pause, STATUS_FILE_PAUSE, '0') != 0) {
		printf("Could not initialize status file %s\n", STATUS_FILE_PAUSE);
		return -1;
	}

	if (init_status_file(&device.ffwd, STATUS_FILE_FFWD, '0') != 0) {
		printf("Could not initialize status file %s\n", STATUS_FILE_FFWD);
		return -1;
	}

	return 0;
}

void update_status(void)
{
	device.play.status = (device.play.filemap[0] != '0');
	device.pause.status = (device.pause.filemap[0] != '0');
	device.ffwd.status = (device.ffwd.filemap[0] != '0');
}

void close_status_files(void)
{
	munmap(device.play.filemap, 1);
	munmap(device.pause.filemap, 1);
	munmap(device.ffwd.filemap, 1);

	close(device.play.fd);
	close(device.pause.fd);
	close(device.ffwd.fd);
}

int main(int argc, char **argv)
{
	int err;
	ogg_page og;
	ogg_packet op;
	int eos = 0;
	int serialno;
	unsigned long bytes;
	char *buffer;
	snd_seq_timestamp_t ts;
	snd_seq_event_t ev;
	int c;
	int use_status_files = 0;
	ogg_page *buffered_page = NULL;

	memset(&ts, 0, sizeof(snd_seq_timestamp_t));
	
	device.pcm_device = "default";
	device.seq_addr.client = 64;
	device.seq_addr.port = 0;
	device.fast_forwarding = 0;
	device.play.status = 1;
	device.pause.status = 0;
	device.ffwd.status = 0;
	device.verbose = 0;

	while ((c = getopt(argc, argv, "D:p:sv")) != -1) {
		switch (c) {
		case 'D':
			device.pcm_device = optarg;
			break;
		case 'p':
			snd_seq_parse_address(NULL, &device.seq_addr, optarg);
			break;
		case 's':
			use_status_files = 1;
			break;
		case 'v':
			device.verbose++;
			break;
		default:
			fprintf(stderr, "invalid options %c\n", c);
			return 1;
		}
	}

	open_file(argv[optind]);

	err = open_devices();
	if (err < 0) {
		printf("Coudln't open devices\n");
		return -1;
	}

	err = configure_audio_device();
	if (err < 0) {
		printf("Couldn't configure audio device\n");
		close_devices();
		return -1;
	}

	err = configure_midi_device();
	if (err < 0) {
		printf("Couldn't configure midi device\n");
		close_devices();
		return -1;
	}

	err = sync_devices();
	if (err < 0) {
		printf("Could not sync devices\n");
		close_devices();
		return -1;
	}

	if (use_status_files) {
		err = setup_status_files();
		if (err < 0) {
			printf("Could not open/initialize status files\n");
			close_devices();
			return -1;
		}
	}

	err = set_midi_tempo();
	if (err < 0) {
		printf("Could not set midi tempo\n");
		close_devices();
		return -1;
	}

	snd_seq_start_queue(device.seqhandle, device.q, 0);
	start_queue();

	/****** GUTS *********/

	

	// decode any midi packets that were queued
	while (1) {
		err = ogg_stream_packetout(device.os_m, &op);
		if (err == 0) break; // need more data
		if (err > 0) {	 
			// we have a packet!
			decode_midi_packet(&op);
		}
	}

	// decode any vorbis packets tthat were queued
	while (1) {
		err = ogg_stream_packetout(device.os_v, &op);
		if (err == 0) break; // need more data
		if (err > 0) {
			// we have a packet!
			decode_vorbis_packet(&op);
		}
	}

	// PLAYBACK :)
	while (!eos && device.play.status) {
		/* Pause if we're supposed to */
		while (device.pause.status) {
			usleep(10000);
			update_status();
		}

		/* Handle fast foward */
		if (device.ffwd.status && !device.fast_forwarding) {
			// start fast forward
			device.fast_forwarding = 1;
			snd_seq_change_queue_tempo(device.seqhandle, device.q, device.mi.tempo / FFWD_SPEED, NULL);
			snd_seq_drain_output(device.seqhandle);
		} else if (!device.ffwd.status && device.fast_forwarding) {
			// stop fast forward
			device.fast_forwarding = 0;
			snd_seq_ev_clear(&ev);
			snd_pcm_status(device.pcmhandle, device.pcmstat);

			snd_seq_change_queue_tempo(device.seqhandle, device.q, device.mi.tempo, NULL);
			snd_seq_ev_set_queue_pos_tick(&ev, device.q, (1000000 * (samples_played - snd_pcm_status_get_delay(device.pcmstat)) / 44100) / 
				((double)device.mi.tempo / (double)device.mi.ticks));
			snd_seq_event_output_direct(device.seqhandle, &ev);
			snd_seq_drain_output(device.seqhandle);
		}


		while (!eos && device.play.status) {
			err = ogg_sync_pageout(&device.oy, &og);
			if (err == 0) break; // need more data
			if (err < 0) {
				printf("Corrupt or missing data in bitstream.\n");
				close_devices();
				exit(1);
			}

			serialno = ogg_page_serialno(&og);
			
			if (serialno == device.midi_serialno) {
				ogg_stream_pagein(device.os_m, &og);
			} else if (serialno == device.vorbis_serialno) {
				if (buffered_page == NULL) {
					buffered_page = copy_page(&og);
				} else {
					ogg_stream_pagein(device.os_v, buffered_page);
					free(buffered_page->header);
					free(buffered_page->body);
					free(buffered_page);
					buffered_page = copy_page(&og);
				}
			} else {
				printf("Got a foreign page.\n");
				close_devices();
				exit(1);
			}

			while (1) {
				if (serialno == device.midi_serialno) {
					err = ogg_stream_packetout(device.os_m, &op);
				} else {
					err = ogg_stream_packetout(device.os_v, &op);
				}
				if (err == 0) break; // need more data
				if (err > 0) {
					// we have a packet!
					if (serialno == device.midi_serialno) {
						decode_midi_packet(&op);
					} else {
						decode_vorbis_packet(&op);
					}
				}
			}

			// NOTE: this is wrong.  we should wait for _both_ eos's
			if (ogg_page_eos(&og)) eos = 1;
		}

		if (!eos && device.play.status) {
			buffer = ogg_sync_buffer(&device.oy, 1024);
			bytes = fread(buffer, 1, 1024, device.fp);
			ogg_sync_wrote(&device.oy, bytes);
			if (bytes == 0) eos = 1;
		}

		if (use_status_files) update_status();
	}
	

	close_file();

	if (use_status_files) close_status_files();

	snd_seq_stop_queue(device.seqhandle, device.q, 0);

	close_devices();

	return 0;
}





