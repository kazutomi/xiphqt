/*  Simple multi-file streaming to a server.
 *
 *  Daniel Holth <dholth@fastmail.fm>, 2004
 */

#include "shoutcc.h"
#include <stdio.h>

#define BUFSIZE 4096

void usage(char *name)
{
    printf("Usage:\n"
           "\t%s file1.ogg [file2.ogg]...\n", name);
}


int main(int argc, char **argv)
{
    static unsigned char buffer[BUFSIZE];
    Shout s;

    try {
        try {
            s.set_user("source");
            s.set_password("hackme");
            s.set_mount("/");
            s.set_port(8001);
            s.set_format(SHOUT_FORMAT_VORBIS);

            s.open();
        } 
        catch(ShoutErr error) {
            printf("Error connecting to server %s:%d with  username %s, password %s.\n",
                   s.get_host(), s.get_port(), s.get_user(), s.get_password());
            throw error;
        }

        if(argc > 1) {
            int delay;
            for(int i=1; i<argc; i++) {
                size_t bytes = 0;
                FILE *f = fopen(*(argv+i), "rb");

                while(!feof(f)) {
                    bytes = fread(buffer, sizeof(char), BUFSIZE, f);
                    s.send(buffer, bytes);

                    delay = s.delay();
                    if(delay > 1000) {
                        printf("delay: %d\n", s.delay());
                    }

                    s.sync();
                }
            }
            s.close();
        } 
        else {
            usage(*argv);
        }
    }
    catch(ShoutErr error) {
        printf("Shout Error: %s (code %d)\n", error.err.c_str(), error.ern);
        exit(1);
    }

    return 0;
}
