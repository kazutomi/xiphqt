#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <stdio.h>

#include "xalloc.h"
#include "thread.h"

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
	char *foo, *bar, *baz, *bla;

#ifdef XALLOC_DEBUG
	xalloc_initialize_debug(2, stdout);
#else
	xalloc_initialize();
#endif

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
	baz = xrealloc(baz, 100, sizeof(char));
	xasprintf(&bla, "Blabla? Blah-%d!", 1000);
	xfree(foo);
	xfree(baz);

	thread_shutdown();
	xalloc_shutdown();
	return (0);
}
