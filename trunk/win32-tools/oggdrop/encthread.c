#include <windows.h>
#include <time.h>
#include <string.h>

#include "audio.h"
#include "encode.h"

extern int encoding_done;
extern int animate;
extern double file_complete;
extern int totalfiles;
extern int numfiles;

typedef struct enclist_tag {
	char *filename;
	struct enclist_tag *next;
} enclist_t;

enclist_t *head = NULL;

oe_options opt = {NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, 
		0, 0, NULL,NULL, 64}; /* Default values */

/* Define supported formats here */
input_format formats[] = {
	{wav_open, wav_close, "wav", "WAV file reader"},
	{NULL, NULL, NULL, NULL}
};

CRITICAL_SECTION mutex;

DWORD WINAPI encode_thread(LPVOID arg);

void encthread_init(void)
{
	int thread_id;
	HANDLE thand;

	numfiles = 0;
	totalfiles = 0;
	file_complete = 0.0;

	InitializeCriticalSection(&mutex);

	thand = CreateThread(NULL, 0, encode_thread, NULL, 0, &thread_id);
	if (thand == NULL) {
		// something bad happened, might want to deal with that, maybe...
	}
}

void encthread_addfile(char *file)
{
	char *filename;
	enclist_t *entry, *node;

	if (file == NULL) return;

	// create entry
	filename = strdup(file);
	entry = (enclist_t *)malloc(sizeof(enclist_t));

	entry->filename = filename;
	entry->next = NULL;

	EnterCriticalSection(&mutex);

	// insert entry
	if (head == NULL) {
		head = entry;
	} else {
		node = head;
		while (node->next != NULL)
			node = node->next;

		node->next = entry;
	}
	numfiles++;
	totalfiles++;

	LeaveCriticalSection(&mutex);
}

// the caller is responsible for deleting the pointer
char *_getfile()
{
	char *filename;
	enclist_t *entry;

	EnterCriticalSection(&mutex);

	if (head == NULL) {
		LeaveCriticalSection(&mutex);
		return NULL;
	}

	// pop entry
	entry = head;
	head = head->next;

	filename = entry->filename;
	free(entry);

	LeaveCriticalSection(&mutex);

	return filename;
}

void encthread_setbitrate(int kbps)
{
	opt.kbps = kbps;
}

void _nothing_prog(char *fn, long total, long done, double time)
{
	// do nothing
}

void _nothing_end(char *fn, double time, int rate, long samples, long bytes)
{
	// do nothing
}

void _error(char *errormessage)
{
	// do nothing
}

void _update(char *fn, long total, long done, double time)
{
	file_complete = (double)done / (double)total;
}

DWORD WINAPI encode_thread(LPVOID arg)
{
	char *in_file;
	long nextserial;

	/* Now, do some checking for illegal argument combinations */

	/* We randomly pick a serial number. This is then incremented for each file */
	srand(time(NULL));
	nextserial = rand();

	while (!encoding_done) {
		while (in_file = _getfile()) {
			oe_enc_opt      enc_opts;
			char *out_fn = NULL;
			FILE *in, *out = NULL;
			int foundformat = 0;
			int j=0;
			vorbis_comment vc;

			vorbis_comment_init(&vc);

			animate = 1;

			/* Set various encoding defaults */
			enc_opts.serialno = nextserial++;
			enc_opts.progress_update = _update;
			enc_opts.end_encode = _nothing_end;
			enc_opts.error = _error;

			set_filename(in_file);

			in = fopen(in_file, "rb");

			if (in == NULL) {
				error_handler("Can't open %s", in_file);
				numfiles--;
				continue;
			}

			if (out == NULL) {
				/* Create a filename from existing filename, replacing extension with .ogg */
				char *start, *end;

				start = in_file;
				end = strrchr(in_file, '.');
				end = end?end:(start + strlen(in_file)+1);
			
				out_fn = (char *)malloc(end - start + 5);
				strncpy(out_fn, start, end-start);
				out_fn[end-start] = 0;
				strcat(out_fn, ".ogg");
			}

			/* Now, we need to select an input audio format */

			while (formats[j].open_func) {
				if (formats[j].open_func(in, &enc_opts)) {
					foundformat = 1;
					break;
				}
				j++;
        (void)fseek(in, 0, SEEK_SET);
			}

			if (!foundformat) {
				free(out_fn);
				/* error reported by reader */
				numfiles--;
				continue;
			}

			out = fopen(out_fn, "wb");
			if(out == NULL) {
				error_handler("Error opening/writing to output file %s", out_fn);
				numfiles--;
        (void)fclose(in);
				continue;
			}	

			/* Now, set the rest of the options */	
			enc_opts.out = out;
			enc_opts.comments = &vc;
			enc_opts.filename = out_fn;
			/* enc_opts.bitrate = opt.kbps; /* defaulted at the start, so this is ok */
			enc_opts.bitrate = opt.kbps * enc_opts.channels / 2; /* Olaf: Scale to match channel count */

			if (!enc_opts.total_samples_per_channel)
				enc_opts.progress_update = _nothing_prog;

			oe_encode(&enc_opts); /* Should we care about return val? */

			if (out_fn) free(out_fn);
			formats[j].close_func(enc_opts.readdata);
			fclose(in);
			fclose(out);

			numfiles--;
		} /* Finished this file, loop around to next... */

		file_complete = 0.0;
		animate = 0;
		totalfiles = 0;
		numfiles = 0;

		Sleep(500);
	} 

	/* Olaf: We already reset these a few lines above
	animate = 0;
	totalfiles = 0;
	numfiles = 0;
	*/

	DeleteCriticalSection(&mutex);

	return 0;
}
