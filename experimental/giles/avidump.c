/*
   avidump.c

   simple format dump utility for avi movies

   Copyright (C) 2004 Ralph Giles. All rights reserved.

   Distributed under the terms of the GNU GPL.
   See http://www.gnu.org/ for details.
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FILE *wav;
FILE *mng;
int mng_width, mng_height;
int mng_fps;

#undef MIN
#define MIN(x,y) (((x)>(y))?(y):(x))

/* read a 16 bit little-endian integer */
void read16(FILE *in, uint16_t *p)
{
	unsigned char q[2];
	fread(q, 1, 2, in);
	*p = q[0] | q[1] << 8;
	return;
}

/* read a 32 bit little-endian integer */
void read32(FILE *in, uint32_t *p)
{
	unsigned char q[4];
	fread(q, 1, 4, in);
	*p = q[0] | q[1] << 8 | q[2] << 16 | q[3] << 24;
	return;
}

/* write a 16 bit little-endian integer */
void write16(FILE *out, uint16_t p)
{
	unsigned char q[2];
	q[0] = p & 0xFF;
	q[1] = (p >> 8) & 0xFF;
	fwrite(q, 1, 2, out);
	return;
}

/* write an 8 bit integer */
void write8(FILE *out, unsigned char p)
{
	fwrite(&p, 1, 1, out);
	return;
}

/* write a 32 bit little-endian integer */
void write32(FILE *out, uint32_t p)
{
	unsigned char q[4];
	q[0] = p & 0xFF;
	q[1] = (p >> 8) & 0xFF;
	q[2] = (p >> 16) & 0xFF;
	q[3] = (p >> 24) & 0xFF;
	fwrite(q, 1, 4, out);
	return;
}

/* write a 32 bit big-endian integer */
void write_be32(FILE *out, uint32_t p)
{
	unsigned char q[4];
	q[0] = (p >> 24) & 0xFF;
	q[1] = (p >> 16) & 0xFF;
	q[2] = (p >> 8) & 0xFF;
	q[3] = p & 0xFF;
	fwrite(q, 1, 4, out);
	return;
}

/* insert a 32 bit big-endian integer into a buffer */
void put_be32(unsigned char *p, uint32_t q)
{
	p[0] = (q >> 24) & 0xFF;
	p[1] = (q >> 16) & 0xFF;
	p[2] = (q >> 8) & 0xFF;
	p[3] = q & 0xFF;
	return;
}

/* fix the length fields of a wav file */
int wav_finalize(FILE *f)
{
	unsigned long size = ftell(f);
	int status = 0;

	/* set the file length */
	status = fseek(f, 4, SEEK_SET);
	write32(f, size - 8);
	/* set data length */
	status = fseek(f, 40, SEEK_SET);
	write32(f, size - 44);

	return status;
}

/* write the crc for a mng chunk
   assuming we've just finished writing the body */
uint32_t mng_update_crc(uint32_t crc, unsigned char *buf, uint32_t len)
{
	static uint32_t table[256]; /* speed up table of 8 bit crcs */
	static int table_computed = 0; /* remember if table has been computed */
	int i;
	
	/* precompute a table of 8 bit crc results */
	if (!table_computed) {
		uint32_t c;
		int k;
		for (i = 0; i < 256; i++) {
			c = (uint32_t)i;
			for (k = 0; k < 8; k++) {
				if (c & 1)
					c = 0xedb88320 ^ (c >> 1);
				else
					c = c >> 1;
			}
			table[i] = c;
		}
		table_computed = 1;
	}

	/* run our buffer through the crc one byte at a time */
	for (i=0; i < len; i++) {
		crc = table[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
	}

	return crc;
}

/* write the MNG headers for a VLC+JNG stream */
int mng_init(FILE *f, int width, int height, int fps)
{
	const unsigned char sig[8] = {'\212','M','N','G','\r','\n','\032','\n'};
	unsigned char buf[48];
	unsigned char *p;
	uint32_t crc;

	/* record the dimensions */
	mng_width = width;
	mng_height = height;

	memcpy(buf, sig, 8); /* MNG file signature */

	/* MHDR */
	p = buf + 8;
	put_be32(p, 28); /* body length */
	memcpy(p+4, "MHDR", 4); /* chunk type */
	p += 8;
	put_be32(p, width);
	put_be32(p+4, height);
	put_be32(p+8, fps);
	put_be32(p+12, 0); /* nominal layer count */
	put_be32(p+16, 0); /* nominal frame count */
	put_be32(p+20, 0); /* nominal play time */
	put_be32(p+24, 1 | 1 << 4 | 1 << 6); /* profile: VLC + JNG - transparency */
	p += 28;
	crc = mng_update_crc(0xFFFFFFFF, buf+12, 4 + 28) ^ 0xFFFFFFFF;
	fprintf(stderr, "writing %c%c%c%c crc 0x%08x\n",
		buf[12],buf[13],buf[14],buf[15], crc);
	put_be32(p, crc);

	/* write out the data */
	fwrite(buf, 1, 48, f);

	return 0;
}

/* copy a frame into a JNG datastream */
int mng_copy_jng(FILE *f, FILE *in, int len)
{
	unsigned char *buf;
	int s, m, read = 0;
	uint32_t crc;

	/* JHDR */
	s = 4*4;
	buf = malloc(s + 12);
	put_be32(buf, s);
	memcpy(buf+4, "JHDR", 4);
	put_be32(buf+8, mng_width);
	put_be32(buf+8+4, mng_height);
	buf[8+8] = 10; /* color type YCbCr */
	buf[8+9] = 8;  /* sample and q table bits */
	buf[8+10] = 8; /* baseline jpeg compression */
	buf[8+11] = 0; /* not progressive jpeg */
	put_be32(buf+8+12, 0); /* no alpha layer */
	crc = mng_update_crc(0xFFFFFFFF, buf + 4, s + 4) ^ 0xFFFFFFFF;
	put_be32(buf+8+s, crc);
	fwrite(buf, 1, s + 12, f);
	free(buf);

	/* JDAT */
	m = MIN(0x10000,len);
	buf = malloc(m);
	while (read < len) {
		s = MIN(len - read, m);
		fread(buf, 1, s, in);
		write_be32(f, s);   /* chunk size */
		fprintf(f, "JDAT"); /* chunk type */
		fwrite(buf, 1, s, f); /* chunk body */
		crc = 0xFFFFFFFF;
		crc = mng_update_crc(crc, "JDAT", 4);
		crc = mng_update_crc(crc, buf, s);
		crc ^= 0xFFFFFFFF;
		write_be32(f, crc); /* chunk CRC */
		read += s;
	}
	free(buf);

	/* IEND */
	write_be32(f, 0);   /* length */
	fprintf(f, "IEND"); /* type */
	crc = mng_update_crc(0xFFFFFFFF, "IEND", 4) ^ 0xFFFFFFFF;
	write_be32(f, crc); /* CRC */

	return 0;
}


/* finalize mng output */
int mng_finalize(FILE *f)
{
	uint32_t crc;

	/* MEND */
	write_be32(f, 0);   /* length */
	fprintf(f, "MEND"); /* type */
	crc = mng_update_crc(0xFFFFFFFF, "MEND", 4) ^ 0xFFFFFFFF;
	fprintf(stderr, "writing out MEND crc 0x%08x\n", crc);
	write_be32(f, crc); /* crc */

	return 0;
}

int dump_chunk_avih(const char *type, uint32_t len, FILE *in, FILE *out)
{
	uint32_t period, rate;
	uint32_t streams;
	int read = 0;

	read32(in, &period); /* often incorrect */
	read32(in, &rate);   /* (nominal?) bitrate */
	read += 8;
        fseek(in, 4 * 4, SEEK_CUR); /* skip 4 fields */
	read += 16;
	read32(in, &streams);/* number of streams */
	read += 4;

	fprintf(out, "\n   %d streams, nominal period %d nominal bitrate %d",
		streams, period, rate);

	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk_strh(const char *type, uint32_t len, FILE *in, FILE *out)
{
	int read = 0;
	unsigned char subtype[5], codec[5];

	fread(subtype, 1, 4, in); subtype[4] = '\0';
	fread(codec, 1, 4, in); codec[4] = '\0';
	read += 8;

	fprintf(out, " stream header");
	fprintf(out, "\n   %s: %s", subtype, codec);

	if (!strncmp(subtype, "vids", 4)) {
		uint32_t flags;
		uint16_t priority, language;
		int start, count;
		int scale, rate;
		int starttime;

		read32(in, &flags);
		read16(in, &priority);
		read16(in, &language);
		read32(in, &start);
		read32(in, &scale);
		read32(in, &rate);
		read32(in, &starttime);
		read32(in, &count);
		read += 6*4 + 2*2;

		if (!rate && !scale) {
			/* should look at period from avih if it's set */
			fprintf(out, " framerate unspecified\n");
		} else {
			double fps = (double)rate/(double)scale;
			double duration = (double)count/fps;
			fprintf(out, " %.2fs video at %.2f fps", duration, fps);
		}

		if (!scale) scale =  1;
		mng_fps = (int) ((double)rate/(double)scale);
		if (!mng_fps) mng_fps = 15;

		fprintf(out, "\n    flags 0x%08x", flags);
		fprintf(out, "\n    priority %d", priority);
		fprintf(out, "\n    language %d", language);
		fprintf(out, "\n    start %d", start);
		fprintf(out, "\n    scale %d", scale);
		fprintf(out, "\n    rate %d", rate);
		fprintf(out, "\n    starttime %d", starttime);
		fprintf(out, "\n    count %d", count);
	} else if (!strncmp(subtype, "auds", 4)) {
		uint32_t flags;
		uint16_t priority, language;
		int initial, scale, rate, start;
		int length, bufsize, samplesize, quality;

		if (!strncmp(codec, "\0\0\0\0", 4))
			fprintf(out, " (PCM audio)");
		read32(in, &flags);
		read16(in, &priority);
		read16(in, &language);
		read32(in, &initial);
		read32(in, &scale);
		read32(in, &rate);
		read32(in, &start);
		read32(in, &length);
		read32(in, &bufsize);
		read32(in, &quality);
		read32(in, &samplesize);
		read += 2*2 + 9*4;

		fprintf(out, " %d Hz", rate/scale);
		fprintf(out, " %d bits per sample", samplesize*8);
 
		fprintf(out, "\n    flags 0x%08x", flags);
		fprintf(out, "\n    priority %d", priority);
		fprintf(out, "\n    language %d", language);
		fprintf(out, "\n    initial %d", initial);
		fprintf(out, "\n    scale %d", scale);
		fprintf(out, "\n    rate %d", rate);
		fprintf(out, "\n    start %d", start);
		fprintf(out, "\n    length %d", length);
		fprintf(out, "\n    bufsize %d", bufsize);
		fprintf(out, "\n    quality %d", quality);
		fprintf(out, "\n    samplesize %d", samplesize);
	}
	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk_strf(const char *type, uint32_t len, FILE *in, FILE *out)
{
	static int called = 0; /* we're called first for video, then for audio */
	int read = 0;

	if (!called) {
		/* video stream format */
		uint32_t size, width, height;
		uint16_t panes, depth;
		unsigned char tag[5];
		int imagesize, xres, yres;

		read32(in, &size);
		read32(in, &width);
		read32(in, &height);
		read16(in, &panes);
		read16(in, &depth);
		fread(tag, 1, 4, in); tag[4] = '\0';
		read32(in, &imagesize);
		read32(in, &xres);
		read32(in, &yres);
		read += 8*4;

		fprintf(out, " %dx%d images, %d bits per pixel",
			width, height, depth);
		fprintf(out, "\n    size %d", size);
		fprintf(out, "\n    tag '%s'", tag);
		fprintf(out, "\n    panes %d", panes);
		fprintf(out, "\n    image size %d", imagesize);
		fprintf(out, "\n    resolution %dx%d pixels per meter", xres, yres);

		called++;

		mng_init(mng, width, height, mng_fps);
	} else {
		/* audio stream format */
		uint16_t format, channels;
		int samplerate, datarate;
		uint16_t blockalign, bits;

		read16(in, &format);
		read16(in, &channels);
		read32(in, &samplerate);
		read32(in, &datarate); /* bytes per second */
		read16(in, &blockalign);
		read16(in, &bits); /* bits per sample */
		read += 16;

		fprintf(out, " %d channel %d Hz %d bit audio",
			channels, samplerate, bits);
		fprintf(out, "\n    format %d", format);
		switch (format) {
			case 0x0001: fprintf(out, " (PCM audio)"); break;
			case 0x0101: fprintf(out, " (muLAW)"); break;
			case 0x0102: fprintf(out, " (IBM aLAW)"); break;
			case 0x0103: fprintf(out, " (IBM ADPCM)"); break;
			default: fprintf(out, " (unknown)"); break;
		} 
		fprintf(out, " blockalign %d", blockalign);
		fprintf(out, "\n    datarate %d bytes per second", datarate);

		/* dump wav header to separate file */
		fprintf(wav, "RIFF");
		write32(wav, 0); /* file body length */
		fprintf(wav, "WAVE");
		fprintf(wav, "fmt ");
		write32(wav, 16); /* format chunk body length */
		write16(wav, format);
		write16(wav, channels);
		write32(wav, samplerate);
		write32(wav, datarate);
		write16(wav, blockalign);
		write16(wav, bits);
		fprintf(wav, "data");
		write32(wav, 0);
	}

	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk_IDIT(const char *type, uint32_t len, FILE *in, FILE *out)
{
	int read = 0;
	char *string;

	string = malloc(len + 1);
	if (string) {
		fread(string, 1, len, in);
		read += len;
		string[len] = '\0'; /* seem to be null terminated, but be sure */
		fprintf(out, " \"%s\"", string);
		free(string);
	}

	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk_ISFT(const char *type, uint32_t len, FILE *in, FILE *out)
{
	int read = 0;
	char*string;

	string = malloc(len + 1);
	if (string) {
		fread(string, 1, len, in);
		read += len;
		string[len] = '\0';
		fprintf(out, " \"%s\"", string);
		free(string);
	}

	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk_xxwb(const char *type, uint32_t len, FILE *in, FILE *out)
{
	int read = 0;
	unsigned char buf[4096];
	int s;

	fprintf(out, " audio frame data.");

	/* copy data to a separate file */
	while (read < len) {
		s = MIN(len - read, 4096);
		fread(buf, 1, s, in);
		fwrite(buf, 1, s, wav);
		read += s;
	}

	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk_xxdc(const char *type, uint32_t len, FILE *in, FILE *out)
{
	int read = 0;
	fprintf(out, " video frame data.");

	mng_copy_jng(mng, in, len);
	read += len;

	return fseek(in, len-read, SEEK_CUR);
}


int dump_chunk_xxdb(const char *type, uint32_t len, FILE *in, FILE *out)
{
	int read = 0;
	fprintf(out, " uncompressed bitmap data.");
	return fseek(in, len-read, SEEK_CUR);
}

int dump_chunk(const char* type, FILE *in, FILE *out)
{
	uint32_t len;
	int status = 0;

	read32(in, &len);
	fprintf(out, "  %s (%d bytes)", type, len);
	if (!strncmp(type+2, "wb", 2))
		status = dump_chunk_xxwb(type, len, in, out);
	else if (!strncmp(type+2, "dc", 2))
		status = dump_chunk_xxdc(type, len, in, out);
	else if (!strncmp(type+2, "db", 2))
		status = dump_chunk_xxdb(type, len, in, out);
	else if (!strncmp(type, "avih", 4))
		status = dump_chunk_avih(type, len, in,out);
	else if (!strncmp(type, "strh", 4))
		status = dump_chunk_strh(type, len, in,out);
	else if (!strncmp(type, "strf", 4))
		status = dump_chunk_strf(type, len, in,out);
	else if (!strncmp(type, "IDIT", 4))
		status = dump_chunk_IDIT(type, len, in,out);
	else if (!strncmp(type, "ISFT", 4))
		status = dump_chunk_ISFT(type, len, in,out);
	else
		/* skip over chunk body data */
		status = fseek(in, len, SEEK_CUR);
	fprintf(out, "\n");

	return status;
}

int dump_list(FILE *in, FILE *out)
{
	unsigned char type[5];
	uint32_t len;

	read32(in, &len);
	fread(type, 1, 4, in); type[4] = '\0';
	fprintf(out, " LIST %s (%d bytes):\n", type, len);
	return 0;
}

int dispatch(FILE *in, FILE *out)
{
	unsigned char sig[5];
	int read;

	read = fread(sig, 1, 4, in); sig[4] = '\0';
	if (!read) return 0;
	if (read != 4) return 1;
	if (!strncmp(sig, "LIST", 4)) dump_list(in, out);
	else dump_chunk(sig, in, out);
	return 0;
}

int dump_avi(FILE *in, FILE *out)
{
	char riff[4],fmt[5];
	uint32_t len;
	int status;

	fread(riff, 1, 4, in);
	if (strncmp(riff, "RIFF", 4)) return 1;
	read32(in, &len);
	fread(fmt, 1, 4, in); fmt[4] = '\0';
	fprintf(out, "RIFF %s (%d bytes)\n", fmt, len);

	while (!feof(in)) {
		status = dispatch(in, out);
		if (status) return status;
	}

	return 0;
}

void usage(const char *name, FILE *out)
{
	fprintf(out, "usage: %s <file1.avi> [<file2.avi> ...]\n", name);
	fprintf(out, "  dumps the structure of an AVI file for inspection.\n");
	return;
}

int main(int argc, char *argv[])
{
	int i;
	FILE *in;

	if (argc < 2) usage(argv[0], stderr);

	for (i = 1; i < argc; i++) {
		in = fopen(argv[1], "rb");
		if (in != NULL) {
			wav = fopen("out.wav", "wb");
			mng = fopen("out.mng", "wb");
			fprintf(stdout, "dump of '%s':\n", argv[i]);
			if (dump_avi(in,stdout))
				fprintf(stderr, "error parsing '%s'\n", argv[i]);
			fclose(in);
			wav_finalize(wav);
			fclose(wav);
			mng_finalize(mng);
			fclose(mng);
		} else {
			fprintf(stderr, "could not open '%s'\n", argv[i]);
		}
	}

	return 0;
}
