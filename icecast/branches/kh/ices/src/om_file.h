#ifndef __OM_FILE_H
#define __OM_FILE_H

struct output_file_stream
{
    int fd;
    char *savefilename;
    mode_t fmask;
    mode_t dmask;
    unsigned long count;
    unsigned saveduration;
    time_t save_start;
    /* int reset; */
    int close_on_new_headers;
    int last_packet_exists; 
    ogg_packet last_packet;
};

int parse_savefile (xmlNodePtr node, void *arg);

#endif  /* __OM_FILE_H */
