/* example.c: Demonstration of the libshout API. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <shout/shout.h>

shout_t *shout;

int try()
{
	int ret;

	if (shout_set_host(shout, "localhost") != SHOUTERR_SUCCESS) {
		printf("Error setting hostname: %s\n", shout_get_error(shout));
		return 1;
	}
	shout_set_port(shout, 8000);
	if (shout_set_password(shout, "hackme") != SHOUTERR_SUCCESS) {
		printf("Error setting password: %s\n", shout_get_error(shout));
		return 1;
	}
	if (shout_set_mount(shout, "/example.ogg") != SHOUTERR_SUCCESS) {
		printf("Error setting mount: %s\n", shout_get_error(shout));
		return 1;
	}
	/* shout_set_format(shout, SHOUT_FORMAT_MP3); */
    shout_set_nonblocking (shout, 1);

    while (1)
    {
        ret = shout_open(shout) == SHOUTERR_SUCCESS;
        if (ret == SHOUTERR_SUCCESS)
        {
            /* printf("Connected to server...\n"); */
            break;
        } else if (ret == SHOUTERR_PENDING)
            printf ("pending\n");
        else
        {
            printf("Error connecting: %s\n", shout_get_error(shout));
        }
        usleep (300);
    }

    shout_close(shout);

	return 0;
}

int main()
{
    int countdown = 100; 

	if (!(shout = shout_new()))
    {
		printf("Could not allocate shout_t\n");
		return 1;
	}

    while (--countdown)
    {
        try ();
        usleep(100);
    }
    shout_free (shout);

    return 0;
}

