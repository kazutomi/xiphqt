// OggVorbis plugin for WinAmp and compatible media player
// Copyright 2000 Jack Moffitt <jack@icecast.org> 
//			and Michael Smith <msmith@labyrinth.net.au>
// HTTP streaming support by Aaron Porter <aaron@javasource.org>
// Licensed under terms of the LGPL

#include <windows.h>
#include <mmreg.h>
#include <msacm.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#include <vorbis/vorbisfile.h>
//#include <vorbis/os_types.h>

#include "httpstream.h"

#include "in2.h"


// post this to the main window at end of file (after playback has stopped)
#define WM_WA_MPEG_EOF WM_USER + 2

#define UTF8_ILSEQ	-1

In_Module mod; // the output module (declared near the bottom of this file)
char lastfn[MAX_PATH]; // currently playing file (used for getting info on the current file)
int file_length; // file length, in ms
double decode_pos_ms; // current decoding position, in milliseconds
int paused; // are we paused?
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
char *sample_buffer; // sample buffer

int samplerate;
int num_channels;
int bitrate;
int current_section = -1;
OggVorbis_File input_file; // input file handle

int killDecodeThread=0;					// the kill switch for the decode thread
HANDLE thread_handle=INVALID_HANDLE_VALUE;	// the handle to the decode thread

void DecodeThread(void *b); // the decode thread procedure

void * btdvp=0;

void config(HWND hwndParent)
{
	MessageBox(hwndParent,
		"No configuration. .OGG files are OGG files :):",
		"Configuration",MB_OK);
	// if we had a configuration we'd want to write it here :)
}

void about(HWND hwndParent)
{
	MessageBox(hwndParent,"OggVorbis Player, by Jack Moffitt <jack@icecast.org>\n\tand Michael Smith <msmith@labyrinth.net.au>\n\nHTTP streaming by Aaron Porter <aaron@javasource.org>","About OggVorbis Player",MB_OK);
}

void init() 
{
	httpInit();
}

void quit() 
{ 
	httpShutdown();
}

int isourfile(char *fn) 
{
    setHttpVars();

    return isOggUrl(fn);
} 

/* Converts a UTF-8 character sequence to a UCS-4 character */
int _utf8_to_ucs4(unsigned int *target, const char *utf8, int n)
{
	unsigned int result = 0;
	int count;
	int i;

	/* Determine the number of characters in sequence */
	if ((*utf8 & 0x80) == 0)
		count = 1;
	else if ((*utf8 & 0xE0) == 0xC0)
		count = 2;
	else if ((*utf8 & 0xF0) == 0xE0)
		count = 3;
	else if ((*utf8 & 0xF8) == 0xF0)
		count = 4;
	else if ((*utf8 & 0xFC) == 0xF8)
		count = 5;
	else if ((*utf8 & 0xFE) == 0xFC)
		count = 6;
	else
		return UTF8_ILSEQ; /* Invalid start byte */

	if (n < count)
		return UTF8_ILSEQ; /* Not enough characters */

	if (count == 2 && (*utf8 & 0x1E) == 0)
		return UTF8_ILSEQ; /* Overlong sequence */

	/* Convert the first character */
	if (count == 1)
		result = *utf8;
	else
		result = (0xFF >> (count +1)) & *utf8;

	/* Convert the continuation bytes */
	for (i = 1; i < count; i++)
	{
		if ((utf8[i] & 0xC0) != 0x80)
			return UTF8_ILSEQ; /* Not a continuation byte */
		if (result == 0 &&
			i == 2 &&
			((utf8[i] & 0x7F) >> (7 - count)) == 0)
			return UTF8_ILSEQ; /* Overlong sequence */
		result = (result << 6) | (utf8[i] & 0x3F);
	}

	if (target != 0)
		*target = result;

	return count;
}

/* Converts a UTF-8 string to a WCHAR string */
int UTF8ToWideChar(LPWSTR target, LPCSTR utf8, WCHAR unknown)
{
	int wcount = 0;
	int conv;
	unsigned int ucs4;
	int count = lstrlenA(utf8) +1;

	while (count != 0)
	{
		conv = _utf8_to_ucs4(&ucs4, utf8, count);
		if (conv == UTF8_ILSEQ) return UTF8_ILSEQ;
		if (target != 0)
		{
			if (ucs4 > 0xFFFF)
				*target = unknown; /* Can only handle BMP */
			else
				*target = (WCHAR) ucs4;
			target++;
		}
		wcount++;
		count -= conv;
		utf8 += conv;
	}

	return wcount;
}

size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	HANDLE file = (HANDLE)datasource;
	unsigned long bytesread;

	if(ReadFile(file, ptr, (unsigned long)(size*nmemb), &bytesread, NULL))
	{
		return bytesread/size;
	}

	return 0; /* It failed */
}

int seek_func(void *datasource, ogg_int64_t offset, int whence)
{ /* Note that we still need stdio.h even though we don't use stdio, 
   * in order to get appropriate definitions for SEEK_SET, etc.
   */

	HANDLE file = (HANDLE)datasource;
	int seek_type;
	unsigned long retval;
	int seek_highword = (int)(offset>>32);

	switch(whence)
	{
		case SEEK_SET:
			seek_type = FILE_BEGIN;
			break;
		case SEEK_CUR:
			seek_type = FILE_CURRENT;
			break;
		case SEEK_END:
			seek_type = FILE_END;
			break;
	}

	/* On failure, SetFilePointer returns 0xFFFFFFFF, which is (int)-1 */
	
	retval=SetFilePointer(file, (int)(offset&0xffffffff), &seek_highword, seek_type);

	if(retval == 0xFFFFFFFF)
		return -1;
	else
		return 0; /* Exactly mimic stdio return values */
}

int close_func(void *datasource)
{
	HANDLE file = (HANDLE)datasource;

	return (CloseHandle(file)?0:EOF); /* Return value meaning is inverted from fclose() */
}

long tell_func(void *datasource)
{
	HANDLE file = (HANDLE)datasource;

	return (long)SetFilePointer(file, 0, NULL, FILE_CURRENT); /* This returns the right number */
}


int play(char *fn) 
{ 
	int maxlatency;
	HANDLE stream;
	vorbis_info *vi = NULL;
	ov_callbacks callbacks = {read_func, seek_func, close_func, tell_func};

    setHttpVars();

	if (isOggUrl(fn))
	{
        mod.is_seekable = FALSE;

		if ((btdvp = httpStartBuffering(fn, &input_file, TRUE)) == 0)
            return -1;
	}
    else
    {
        mod.is_seekable = TRUE;

	    stream = CreateFile(fn, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	    if (stream == INVALID_HANDLE_VALUE)
	    {	
		    return -1;
	    }

	    if (ov_open_callbacks(stream, &input_file, NULL, 0, callbacks) < 0) {
		    CloseHandle(stream);
		    return 1;
	    }
    }

	file_length = (int)ov_time_total(&input_file, -1) * 1000;
	strcpy(lastfn, fn);
	paused = 0;
	decode_pos_ms = 0;
	seek_needed = -1;

	vi = ov_info(&input_file, -1);
	samplerate = vi->rate;
	num_channels = vi->channels;
	bitrate = ov_bitrate(&input_file, -1);

	if(num_channels > 2) /* We can't handle this */
	{
		ov_clear(&input_file);
		return 1;
	}

	// allocate the sample buffer - it's twice as big as we apparently need,
	// because mod.dsp_dosamples() may use up to twice as much space. 
	sample_buffer = malloc(576 * num_channels * 2 * 2 ); 
	
	if (sample_buffer == NULL)
	{
		ov_clear(&input_file);
		return 1;
	}

	maxlatency = mod.outMod->Open(samplerate, num_channels, 16, -1, -1);
	if (maxlatency < 0) {
		// error opening device
		ov_clear(&input_file);
		return 1;
	}
	
	// dividing by 1000 for the first parameter of setinfo makes it
	// display 'H'... for hundred.. i.e. 14H Kbps.
	mod.SetInfo(bitrate / 1000, samplerate / 1000, num_channels, 1);

	// initialize vis stuff
	mod.SAVSAInit(maxlatency, samplerate);
	mod.VSASetInfo(samplerate, num_channels);

	mod.outMod->SetVolume(-666); // set the output plug-ins default volume

	killDecodeThread = 0;
	thread_handle = (HANDLE)_beginthread(DecodeThread,0, (void *)(&killDecodeThread) );
	
	return 0; 
}

void pause() 
{
	paused = 1; 
	mod.outMod->Pause(1); 
}

void unpause() 
{ 
	paused = 0; 
	mod.outMod->Pause(0); 
}

int ispaused() 
{ 
	return paused; 
}

void stop() 
{ 
	if (btdvp)
    {
		httpStopBuffering(btdvp);
        btdvp = 0;
    }

	if (thread_handle != INVALID_HANDLE_VALUE) {
		killDecodeThread = 1;
		
		if (WaitForSingleObject(thread_handle, INFINITE) == WAIT_TIMEOUT) {
			MessageBox(mod.hMainWindow, "error asking thread to die!\n", "error killing decode thread", 0);
			TerminateThread(thread_handle, 0);
		}
		ov_clear(&input_file);
		thread_handle = INVALID_HANDLE_VALUE;

	}

	// deallocate sample buffer
	if (sample_buffer)
		free(sample_buffer);

	mod.outMod->Close();

	mod.SAVSADeInit();
}

int getlength() { 
	return file_length; 
}

int getoutputtime() { 
	return ((long)decode_pos_ms) + (mod.outMod->GetOutputTime() - mod.outMod->GetWrittenTime()); 
}

void setoutputtime(int time_in_ms) { 
	seek_needed = time_in_ms; 
}

void setvolume(int volume) 
{ 
	mod.outMod->SetVolume(volume); 
}

void setpan(int pan) 
{ 
	mod.outMod->SetPan(pan); 
}

int infoDlg(char *fn, HWND hwnd)
{
	MessageBox(mod.hMainWindow, "Sorry, there is currently no interface to set or read information.", "Unimplemented", MB_OK);
	// TODO: implement info dialog. 
	return 0;
}

char *generate_title(vorbis_comment *comment, char *fn)
{/* Later, extend this to be configurable like the mp3 player */
	int len;
	char buff[1024];
	char *title, *artist, *finaltitle;
	LPWSTR titleW, artistW;
	LPSTR titleL = NULL, artistL = NULL;

	title = vorbis_comment_query(comment, "title", 0);
	artist = vorbis_comment_query(comment, "artist", 0);

	if (title)
	{
		/* Convert the UTF-8 title to the system code page */
		len = UTF8ToWideChar(NULL, title, '?');
		if (len == UTF8_ILSEQ)
		{
			/* Fallback to ascii */
			titleL = strdup(title);
		}
		else
		{
			/* Convert the UTF-8 string */
			titleL = calloc(len, sizeof(CHAR));
			titleW = calloc(len, sizeof(WCHAR));
			UTF8ToWideChar(titleW, title, '?');
			WideCharToMultiByte(CP_ACP, 0, titleW, -1, titleL,
			                    len, "?", NULL);
			free(titleW);
		}
	}

	if (artist)
	{
		/* Convert the UTF-8 artist to the system code page */
		len = UTF8ToWideChar(NULL, artist, '?');
		if (len == UTF8_ILSEQ)
		{
			/* Fallback to ascii */
			artistL = strdup(artist);
		}
		else
		{
			/* Convert the UTF-8 string */
			artistL = calloc(len, sizeof(CHAR));
			artistW = calloc(len, sizeof(WCHAR));
			UTF8ToWideChar(artistW, artist, '?');
			WideCharToMultiByte(CP_ACP, 0, artistW, -1, artistL,
			                    len, "?", NULL);
			free(artistW);
		}
	}


	if(artist && title)
		_snprintf(buff, 1024, "%s - %s", artistL, titleL);
	else if(title)
		_snprintf(buff, 1024, "%s", titleL);
	else if(artist)
		_snprintf(buff, 1024, "%s - unknown", artistL);
	else
    {
	    if (title = httpGetTitle(fn))
		    return title;

		_snprintf(buff, 1024, "%s (no title)", fn);
    }

	free(artistL);
	free(titleL);
	finaltitle = strdup(buff);

	return finaltitle;
}

void getfileinfo(char *filename, char *title, int *length_in_ms)
{
	HANDLE stream;
	OggVorbis_File vf;
	vorbis_comment *comment;
	ov_callbacks callbacks = {read_func, seek_func, close_func, tell_func};


	if (filename != NULL && filename[0] != 0)
    {
        if (isOggUrl(filename))
        {
            if (!httpStartBuffering(filename, &vf, FALSE))
                return;
        }
        else
        {
		    stream = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		    if(stream == INVALID_HANDLE_VALUE)
			    return;

		    // The ov_open() function performs full stream detection and machine
		    // initialization.  If it returns 0, the stream *is* Vorbis and we're
		    // fully ready to decode.
		    

		    if (ov_open_callbacks(stream, &vf, NULL, 0, callbacks) < 0) {
			    CloseHandle(stream);
			    return;
		    }
        }

		file_length = (int)ov_time_total(&vf, -1) * 1000;
		*length_in_ms = file_length;

		comment = ov_comment(&vf, -1);
		if(comment)
		{
			char *gen_title = generate_title(comment, filename);
			if(gen_title)
			{
				strcpy(title, gen_title);
				free(gen_title);
			}
		}
		else
			strcpy(title,filename);

		// once the ov_open() succeeds, the file belongs to vorbisfile.
		// ov_clear() will close it.
	
		ov_clear(&vf);
		
	} else {
		/* This is the only section of code which uses vorbisfile calls 
		   in one thread whilst the main playback thread is running. 
		   Technically, we should protect it with critical sections, but
		   these two calls appear to be safe on win32/x86 */

		comment = ov_comment(&input_file, -1);
		if(comment)
		{
			char *gen_title = generate_title(comment, lastfn);
			if(gen_title)
			{
				strcpy(title, gen_title);
				free(gen_title);
			}
		}
   		else
			strcpy(title, filename);

		*length_in_ms = (int)ov_time_total(&input_file, -1) * 1000;

	}
}

void eq_set(int on, char data[10], int preamp) 
{ 
	/* Waiting on appropriate libvorbis API additions. */
}


// render 576 samples into buf. 
// note that if you adjust the size of sample_buffer, for say, 1024
// sample blocks, it will still work, but some of the visualization 
// might not look as good as it could. Stick with 576 sample blocks
// if you can, and have an additional auxiliary (overflow) buffer if 
// necessary.. 
int get_576_samples(char *buf)
{
	int ret;

	ret = ov_read(&input_file, buf, 576 * num_channels * 2, 0, 2, 1, &current_section);

	return ret;
}

void DecodeThread(void *b)
{
	int eos = 0;
	int lostsync = 0;
	double lastupdate = 0;

	while (!*((int *)b)) {
		if (seek_needed != -1) {
			decode_pos_ms = (double)seek_needed;// - (seek_needed % 1000);
			lastupdate = decode_pos_ms;
			seek_needed = -1;
			eos = 0;
			mod.outMod->Flush((long)decode_pos_ms);

			ov_time_seek(&input_file, decode_pos_ms / 1000);
		}

		if (eos) {
			mod.outMod->CanWrite();
			
			if (!mod.outMod->IsPlaying()) {
				PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				return;
			}
			
			Sleep(10);
		} else if (mod.outMod->CanWrite() >= ((576 * num_channels * 2) << (mod.dsp_isactive() ? 1 : 0))) {	
			int ret;
			ret = get_576_samples(sample_buffer);
			
			if (ret == 0) {
				// eof
				eos = 1;
			}
			else if(ret < 0)
			{
				/* Hole in data, lost sync, or something like that */
				/* Inform winamp that we lost sync */
				mod.SetInfo(bitrate / 1000, samplerate / 1000, num_channels, 0);
				lostsync = 1;
			}
			else {
				/* Update current bitrate (not currently implemented), and set sync (if lost) */
				if(lostsync || (decode_pos_ms - lastupdate > 500))
				{
					bitrate = ov_bitrate_instant(&input_file);
					mod.SetInfo(bitrate / 1000, samplerate / 1000, num_channels, 1);
					lostsync = 0;
					lastupdate = decode_pos_ms;
				}
				mod.SAAddPCMData((char *)sample_buffer, num_channels, 16, (long)decode_pos_ms);
				mod.VSAAddPCMData((char *)sample_buffer, num_channels, 16, (long)decode_pos_ms);
				decode_pos_ms += (ret/(2*num_channels) * 1000) / (float)samplerate;
								
				if (mod.dsp_isactive()) 
					ret = mod.dsp_dosamples((short *)sample_buffer, ret / num_channels / (2), 16, num_channels, samplerate) * (num_channels * (2));
				
				mod.outMod->Write(sample_buffer, ret);
			}
		} else {
			Sleep(20);
		}
	}
	
	_endthread();
}

In_Module mod = 
{
	IN_VER,
	"OggVorbis Input Plugin 0.2",
	0,	// hMainWindow
	0,  // hDllInstance
	"OGG\0OggVorbis File (*.OGG)\0",
	1,	// is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,
	
	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0, // vis stuff


	0,0, // dsp
 
	eq_set,

	NULL,		// setinfo

	0 // out_mod

};

__declspec( dllexport ) In_Module * winampGetInModule2()
{
	return &mod;
}
