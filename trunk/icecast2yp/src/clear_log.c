#include <stdio.h>
main()
{
	FILE *filep;

	filep = fopen(YP_LOGDIR"yp_cgi.log", "w");
	fclose(filep);
	exit(0);
}
