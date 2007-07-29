#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#include "xalloc.h"

int
main(void)
{
	char *foo, *bar, *baz;

#ifdef XALLOC_DEBUG
	xalloc_initialize_debug(2, NULL);
#else
	xalloc_initialize();
#endif

	foo = xstrdup("FooFooFooFooFoo!");
	bar = xcalloc(12, sizeof(char));
	baz = xmalloc(80);
	baz = xrealloc(baz, 100, sizeof(char));
	xfree(foo);
	xfree(baz);

	xalloc_shutdown();

	return (0);
}
