#include <ogg/ogg.h>
#include <stdio.h>
#include <stdlib.h>

void write_ogg(FILE *out) 
{
    ogg_packet packet;
    ogg_page page;
    int sizes[] = {10, 100, 1000, 10000, -1};
    int i = 0;
    ogg_int64_t current_granulepos = 0;
    ogg_stream_state stream;
    int serial = 123456;

    ogg_stream_init(&stream, serial);

    while(sizes[i] >= 0) {
        int size = sizes[i];
        fprintf(stderr, "Doing size %d\n", size);

        packet.b_o_s = (i == 0);
        packet.e_o_s = sizes[i+1] < 0;
        packet.packet = calloc(1, size);
        packet.bytes = size;
        packet.packetno = i;
        packet.granulepos = current_granulepos;
        
        current_granulepos += 1;

        fprintf(stderr, "Submitting packet with GP %lld\n", packet.granulepos);
        ogg_stream_packetin(&stream, &packet);

        while(ogg_stream_pageout(&stream, &page)) 
        {
            fprintf(stderr, "Writing page %ld\n", ogg_page_pageno(&page));
            fwrite(page.header, 1, page.header_len, out);
            fwrite(page.body, 1, page.body_len, out);
        }

        i++;
    }

    ogg_stream_clear(&stream);
}

int main(int argc, char **argv)
{
    FILE *out;

    if(argc < 2) {
        fprintf(stderr, "Usage: make-ogg out.ogg\n");
        exit(1);
    }

    out = fopen(argv[1], "wb");

    if(!out) {
        fprintf(stderr, "Couldn't write to %s\n", argv[1]);
        exit(1);
    }

    write_ogg(out);
    fclose(out);

    return 0;
}

    
