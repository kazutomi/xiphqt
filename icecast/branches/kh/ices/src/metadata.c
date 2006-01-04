/* metadata.c
 * - Metadata manipulation
 *
 * $Id: metadata.c,v 1.6 2002/07/20 12:52:06 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "cfgparse.h"
#include "inputmodule.h"

#define MODULE "metadata/"
#include "logging.h"

volatile int metadata_update_signalled = 0;


void metadata_thread_signal(input_module_t *mod, input_buffer *buffer)
{
	static char line[1024];
    char **md = NULL;
    int comments = 0;
    FILE *file;

    metadata_update_signalled = 0;

    if (mod->metadata_filename == NULL)
        return;
    file = fopen(mod->metadata_filename, "r");
    if (file == NULL) 
    {
#ifdef STRERROR_R_CHAR_P
        char *buf = strerror_r (errno, line, sizeof (line));
#else
        int i = strerror_r (errno, line, sizeof (line));
#endif
        LOG_WARN2("Failed to open file \"%s\" for metadata update: %s", 
                mod->metadata_filename, line);
        return;
    }

    while(fgets(line, 1024, file))
    {
        if(line[0] == '\n')
            break;
        else
        {
            char **old_buf;
            unsigned len = strlen(line);

            if(line[len-1] == '\n')
                line[len-1] = '\0';
            old_buf = md;
            md = realloc(md, (comments+2)*sizeof(char *));
            if (md)
            {
                md[comments] = malloc(len+1);
                strcpy(md[comments], line);
                comments++;
            }
            else
                md = old_buf;
        }
    }

    fclose(file);

    if(md) /* Don't update if there's nothing there */
    {
        md[comments]=NULL;

        /* Now, let's actually use the new data */
        LOG_INFO1("Updating metadata with %d comments", comments);
        buffer->metadata = md;
    }

}


void metadata_update(char **md, vorbis_comment *vc)
{
    if(md)
    {
        while(*md)
        {
            LOG_INFO1 ("Adding comment %s", *md);
            vorbis_comment_add(vc, *md++);
        }
    }
}

void metadata_free (char **md)
{
    if(md)
    {
        char **comment = md;
        while(*comment)
        {
            free(*comment);
            comment++;
        }
        free(md);
    }
}
