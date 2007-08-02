#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <stdio.h>

#include "xalloc.h"
#include "thread.h"

void *	hello(void *);

void *
hello(void *arg)
{
	printf("Hello World!\n");
	return (arg);
}

int
main(void)
{
	thread_t thread;
	char *foo, *bar, *baz;
#ifdef XALLOC_WITH_XASPRINTF
	char *bla;
#endif

	xalloc_initialize_debug(1, stdout);
#ifdef THREAD_DEBUG
	thread_initialize_debug(3, stdout);
#else
	thread_initialize();
#endif

	thread = thread_create("Test", hello, NULL, THREAD_ATTACHED);

	thread_join(&thread);

	foo = xstrdup("FooFooFooFooFoo!");
	bar = xcalloc(12, sizeof(char));
	baz = xmalloc(80);
	baz = xrealloc(baz, 200, sizeof(char));
	baz = xrealloc(baz, 100, sizeof(char));
#ifdef XALLOC_WITH_XASPRINTF
	xasprintf(&bla, "Blabla? Blah-%d!", 1000);
#endif
	xfree(foo);
	xfree(baz);
	xfree(baz);

	thread_shutdown();
	xalloc_shutdown();
	return (0);
}
