/* midimerge.c
**
** merge a type 1 midi files tracks into one track and output 
** and type 0 file.
**
** usage:
**
** midimerge input.mid output.mid
*/
#include <stdio.h>
#include <sys/types.h>

typedef struct event_tag {
	u_int64_t timestamp;
	
	int event;
	int channel;

	int data1;
	int data2;

	char *data;
	unsigned long datalen;

	struct event_tag *prev;
	struct event_tag *next;
} event_t;

#define HEADER_LEN 14

typedef struct {
	char chunkname[4];
	unsigned long chunklen;
	unsigned short format;
	unsigned short tracks;
	unsigned short timebase;
} header_t;

int is_big_endian(void)
{
        unsigned short pattern = 0xbabe;
        unsigned char *bytewise = (unsigned char *)&pattern;

        if (bytewise[0] == 0xba) return 1;
        return 0;
}

unsigned short swap_short(unsigned short data)
{
	unsigned short ret;
	unsigned char *dataptr = (unsigned char *)&data;
	unsigned char *retptr = (unsigned char *)&ret;

	retptr[0] = dataptr[1];
	retptr[1] = dataptr[0];

	return ret;
}

unsigned long swap_long(unsigned long data)
{
	unsigned long ret;
	unsigned char *dataptr = (unsigned char *)&data;
	unsigned char *retptr = (unsigned char *)&ret;

	retptr[0] = dataptr[3];
	retptr[1] = dataptr[2];
	retptr[2] = dataptr[1];
	retptr[3] = dataptr[0];

	return ret;
}


FILE *open_midifile(char *name, header_t *header)
{
	FILE *fp;
	int read;

	fp = fopen(name, "r");
	if (!fp) return NULL;

	read = fread(header, 1, HEADER_LEN, fp);
	if (read != HEADER_LEN) {
		fclose(fp);
		return NULL;
	}

	if (!is_big_endian()) {
		header->chunklen = swap_long(header->chunklen);
		header->format = swap_short(header->format);
		header->tracks = swap_short(header->tracks);
		header->timebase = swap_short(header->timebase);
	}

	return fp;
}

unsigned long get_var_value(FILE *fp)
{
	unsigned long value = 0;
	unsigned char temp = 0;

	temp = fgetc(fp) & 0xFF;
	value |= (temp & 0x7F);

	if ((temp & 0x80) == 0x80) {
		value <<= 7;
	} else {
		return value;
	}

	temp = fgetc(fp) & 0xFF;
	value |= (temp & 0x7F);

	if ((temp & 0x80) == 0x80) {
		value <<= 7;
	} else {
		return value;
	}

	temp = fgetc(fp) & 0xFF;
	value |= (temp & 0x7F);

	if ((temp & 0x80) == 0x80) {
		value <<= 7;
	} else {
		return value;
	}

	temp = fgetc(fp) & 0x7F;
	value |= temp;

	return value;
}

unsigned long make_var_value(unsigned long orig, int *len)
{
	unsigned long value;
	unsigned char c1 = 0;
	unsigned char c2 = 0;
	unsigned char c3 = 0;
	unsigned char c4 = 0;

	// split into 4 7bit chunks (first 3 have contine bit set)
	c1 = ((orig & 0x0FE00000) >> 21) | 0x80;
	c2 = ((orig & 0x001FC000) >> 14) | 0x80;
	c3 = ((orig & 0x00003F80) >> 7) | 0x80;
	c4 = (orig & 0x0000007F);

	printf("%02x %02x %02x %02x\n", c1, c2, c3, c4);

	// make the var value
	if (c1 & 0x7F) {
		value = (c1 << 24) | (c2 << 16) | (c3 << 8) | c4;
		*len = 4;

		printf("value = %d\tlen = %d\n", value, *len);
	} else if (c2 & 0x7F) {
		value = (c2 << 24) | (c3 << 16) | (c4 << 8);
		*len = 3;

		printf("value = %d\tlen = %d\n", value, *len);
	} else if (c3 & 0x7F) {
		value = (c3 << 24) | (c4 << 16);
		*len = 2;

		printf("value = %d\tlen = %d\n", value, *len);
	} else {
		value = (c4 << 24);
		*len = 1;

		printf("value = %d\tlen = %d\n", value, *len);
	}

	return value;
}

void write_var_value(FILE *fp, unsigned long value, int len)
{
	char c;

	printf("Writing %08x/%d\n", value, len);

	c = (value >> 24) & 0xFF;
	fwrite(&c, 1, 1, fp);

	if (len == 1) return;

	c = (value >> 16) & 0xFF;
	fwrite(&c, 1, 1, fp);

	if (len == 2) return;

	c = (value >> 8) & 0xFF;
	fwrite(&c, 1, 1, fp);

	if (len == 3) return;

	c = value & 0xFF;
	fwrite(&c, 1, 1, fp);
}

void write_midi_file(FILE *fp, event_t *list)
{
	char headchunk[4] = "MThd";
	char trackchunk[4] = "MTrk";
	unsigned long headlen = 6;
	unsigned short format = 0;
	unsigned short tracks = 1;
	unsigned short timebase = 480; // HACK!!!
	unsigned long tracklen = 0xa0b0c0d0;
	event_t *event = NULL;
	unsigned long varstamp = 0;
	int var_len = 0;
	unsigned long realtime = 0;
	unsigned long metalen = 0;
	int metalen_len = 0;
	char trash;

	// write header chunk

	fwrite(headchunk, 1, 4, fp);
	if (!is_big_endian()) headlen = swap_long(headlen);
	fwrite(&headlen, 4, 1, fp);
	if (!is_big_endian()) format = swap_short(format);
	fwrite(&format, 2, 1, fp);
	if (!is_big_endian()) tracks = swap_short(tracks);
	fwrite(&tracks, 2, 1, fp);
	if (!is_big_endian()) timebase = swap_short(timebase);
	fwrite(&timebase, 2, 1, fp);

	// write track chunk

	fwrite(trackchunk, 1, 4, fp);
	// write fake length, which we will replace later
	fwrite(&tracklen, 4, 1, fp);

	
	// now we can start writing event data.
	event = list;
	while (event) {
		// skip end of track events
		// except for last one
		if (event->event == 0xFF && event->data1 == 0x2F && event->next != NULL) {
			realtime = event->timestamp;
			event = event->next;
			continue;
		}

		// write timestamp as delta
		varstamp = make_var_value(event->timestamp - realtime, &var_len);
		printf("writing offset %d ", event->timestamp - realtime);
		printf("as %08x ", varstamp);
		printf("with length %d\n", var_len);
		write_var_value(fp, varstamp, var_len);

		switch (event->event & 0xF0) {
		case 0x80:
		case 0x90:
		case 0xA0:
		case 0xB0:
		case 0xE0:
			// both data bytes
			// channel is already encoded in event
			fwrite(&event->event, 1, 1, fp);
			fwrite(&event->data1, 1, 1, fp);
			fwrite(&event->data2, 1, 1, fp);
			break;
		case 0xC0:
		case 0xD0:
			// one data byte
			// channel is already encoded in event
			fwrite(&event->event, 1, 1, fp);
			fwrite(&event->data1, 1, 1, fp);
			break;
		case 0xF0:
			// metaevent
			switch (event->event) {
			case 0xF0:
				fwrite(&event->event, 1, 1, fp);
				metalen = make_var_value(event->datalen, &metalen_len);
				write_var_value(fp, metalen, metalen_len);
				fwrite(event->data, 1, event->datalen, fp);
				break;
			case 0xFF:
				fwrite(&event->event, 1, 1, fp);
				fwrite(&event->data1, 1, 1, fp);
				metalen = make_var_value(event->datalen, &metalen_len);
				write_var_value(fp, metalen, metalen_len);
				fwrite(event->data, 1, event->datalen, fp);
				break;
			default:
				printf("don't know this event...%02x\n", event->event);
			}
			break;
		default:
			printf("don't know this event %02x\n", event->event);
		}
			


		realtime = event->timestamp;
		event = event->next;
	}
}

int main(int argc, char **argv)
{
	FILE *fp;
	header_t header;
	event_t *event_list = NULL;
	event_t *temp;
	int i;
	unsigned long len;

	fp = open_midifile(argv[1], &header);
	
	if (!fp) {
		printf("ERROR: Couldn't open file, or invalid file..\n");
		return -1;
	}

	printf("chunkname = %c%c%c%c\n", header.chunkname[0], header.chunkname[1], header.chunkname[2], header.chunkname[3]);
	printf("chunklength = %d\n", header.chunklen);
	printf("format = %d\n", header.format);
	printf("tracks = %d\n", header.tracks);
	printf("timebase = %d\n", header.timebase);
	printf("\n");


	while (!feof(fp)) {
		char chunkname[4];
		unsigned long chunklen;
		int read;

		u_int64_t current = 0;
		unsigned char status = 0;

		read = fread(chunkname, 1, 4, fp);
		if (read != 4) break;
		read = fread(&chunklen, 1, 4, fp);
		if (read != 4) break;

		if (!is_big_endian()) chunklen = swap_long(chunklen);

		printf("chunkname = %c%c%c%c\n", chunkname[0], chunkname[1], chunkname[2], chunkname[3]);
		printf("chunklen = %d\n", chunklen);
		printf("\n");

		chunklen += ftell(fp);
	        while (ftell(fp) < chunklen) {
			unsigned char data;
			unsigned char data1;
			unsigned char data2;
			unsigned long metalen;
			event_t *event;

			event = (event_t *)malloc(sizeof(event_t));
			event->prev = NULL;
			event->next = NULL;

			printf("EVENT\n-----\n");

			// read the timestamp
			current += get_var_value(fp);
			
			printf("timestamp = %ld\n", current);

			event->timestamp = current;

			// read or assume status
			data = fgetc(fp);
			if ((data & 0x80) == 0x80) {
				status = data;
			} else {
				ungetc(data, fp);
			}
			
			printf("status = %d\n", status);
			
			event->event = status;

			switch (status & 0xF0) {
			case 0x80:
				// note off
				printf("message = note off\n");
				data1 = fgetc(fp);
				data2 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("key = %d\n", data1);
				printf("velocity = %d\n", data2);

				event->channel = status & 0x0F;
				event->data1 = data1;
				event->data2 = data2;
				break;
			case 0x90:
				// note on
				printf("message = note on\n");
				data1 = fgetc(fp);
				data2 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("key = %d\n", data1);
				printf("velocity = %d\n", data2);

				event->channel = status & 0x0F;
				event->data1 = data1;
				event->data2 = data2;
				break;
			case 0xA0:
				// aftertouch
				printf("message = aftertouch\n");
				data1 = fgetc(fp);
				data2 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("key = %d\n", data1);
				printf("pressure = %d\n", data2);

				event->channel = status & 0x0F;
				event->data1 = data1;
				event->data2 = data2;
				break;
			case 0xB0:
				// controller change
				printf("message = controller change\n");
				data1 = fgetc(fp);
				data2 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("controller = %d\n", data1);
				printf("value = %d\n", data2);

				event->channel = status & 0x0F;
				event->data1 = data1;
				event->data2 = data2;
				break;
			case 0xC0:
				// program change
				printf("message = program change\n");
				data1 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("program = %d\n", data1);

				event->channel = status & 0x0F;
				event->data1 = data1;
				break;
			case 0xD0:
				// channel pressure
				printf("message = channel pressure\n");
				data1 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("pressure = %d\n", data1);

				event->channel = status & 0x0F;
				event->data1 = data1;
				break;
			case 0xE0:
				// pitch bend
				printf("message = pitch bend\n");
				data1 = fgetc(fp);
				data2 = fgetc(fp);
				printf("channel = %d\n", status & 0x0F);
				printf("bend = %02x%02x\n", data2, data1);

				event->channel = status & 0x0F;
				event->data1 = data1;
				event->data2 = data2;
				break;
			case 0xF0:
				// system messages
				switch (status) {
				case 0xF0:
					printf("message = sysex\n");
					metalen = get_var_value(fp);
					printf("length = %d\n", metalen);
					event->datalen = metalen;
					event->data = (char *)malloc(metalen);
					fread(event->data, 1, metalen, fp);
					break;
				case 0xFF:
					printf("message = meta event\n");
					data = fgetc(fp);
					printf("type = %02x\n", data);
					metalen = get_var_value(fp);
					printf("length = %d\n", metalen);
					event->datalen = metalen;
					event->data1 = data;
					event->data = (char *)malloc(metalen);
					fread(event->data, 1, metalen, fp);
					break;
				default:
					printf("status = %02x\nnot implemented yet.. probably sysex\n", status);
				}
				break;
			default:
				printf("something is wrong.. got a bad status\n");
				return -1;
			}

			printf("=====\n");

			// add event to list
			if (event_list == NULL) {
				event_list = event;
			} else {
				temp = event_list;
				while (temp->next != NULL && temp->timestamp < event->timestamp) 
					temp = temp->next;
				event->next = temp->next;
				temp->next = event;
				event->prev = temp;
			}
		}
	}


	fclose(fp);

	i = 0;
	temp = event_list;
	while (temp != NULL) {
		i++;
		temp = temp->next;
	}

	printf("%d events..\n", i);

	// got all the events now, just write out the new file
	// and remember to keep status expanded and to 
	// convert to deltas instead of absolute stamps

	fp = fopen(argv[2], "w");
	if (!fp) {
		printf("Can't open output file.\n");
		return -1;
	}

	write_midi_file(fp, event_list);

	len = ftell(fp);
	len -= 22;
	printf("length of file = %ld/%08x\n", len, len);
	if (!is_big_endian()) len = swap_long(len);
	printf("length of file = %08x\n", len);
	fseek(fp, 18, SEEK_SET);
	fwrite(&len, 4, 1, fp);

	fclose(fp);

	

	return 0;
}







