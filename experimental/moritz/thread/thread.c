/*
 * libthread - Cross platform thread and synchronization library, with
 *             extensive error checking and debugging facility.
 *             Uses POSIX threads (libpthread, or pthread-win32 on
 *             Windows.)
 *
 * Library design and ideas adapted from libicethread, written by
 * Jack Moffitt for the Icecast project, which is Copyright 2000-2004
 * and licensed under the terms of the LGPL - available at
 * http://www.icecast.org/. Licensed and used with permission.
 *
 * Copyright (C) 2007  Moritz Grimm <mgrimm@gmx.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <sys/tree.h>

#include <errno.h>
#ifdef HAVE_POLL_H
# include <poll.h>
#endif
#include <pthread.h>
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
# include <windows.h>
#endif

#include "xalloc.h"
#include "thread.h"


#define MUTEX_LOCKED		 1
#define MUTEX_UNLOCKED		 0
#define MUTEX_NEVERLOCKED	-1

#define THREAD_DBGLVL_MAX	 4
#if defined(THREAD_DEBUG) && defined(THREAD_DIE_SILENT)
# undef THREAD_DIE_SILENT
#endif /* THREAD_DEBUG && THREAD_DIE_SILENT */

void	_thread_fatal(int, const char *, ...);
void	_thread_debug_printf(unsigned int, const char *, ...);
void	_init_mutexes(void);
void	_catch_signals(void);
void	_block_signals(void);
int	_thread_cmp(void *, void *);
int	_mutex_cmp(void *, void *);
int	_cond_cmp(void *, void *);
int	_rwlock_cmp(void *, void *);
void	_thread_free(struct thread **);
void *	_thread_start_routine(void *);
void	_mutex_init(struct mutex *);
void	_mutex_lock(struct mutex *);
void	_mutex_unlock(struct mutex *);
void	_mutex_free(struct mutex **);
void	_cond_free(struct cond **);
void	_rwlock_free(struct rwlock **);


struct thread {
	RB_ENTRY(thread) entry;
	int		 id;
	char		*name;
	time_t		 create_time;
	const char	*creator;
	unsigned int	 line;
	int		 detached;
	pthread_t	 sys_thread;
};
RB_HEAD(thread_tree, thread) thread_tree_head = RB_INITIALIZER(&thread_tree_head);
RB_PROTOTYPE(thread_tree, thread, entry, _thread_cmp)

struct thread_start {
	void *		(*start_routine)(void *);
	void		*start_arg;
	struct thread	*thread;
	pthread_t	 sys_thread;
};

struct mutex {
	RB_ENTRY(mutex)  entry;
	int		 id;
	int		 thread_id;
	char		*name;
	int		 state;
	time_t		 create_time;
	const char	*creator;
	unsigned int	 line;
	pthread_mutex_t  sys_mutex;
};
RB_HEAD(mutex_tree, mutex) mutex_tree_head = RB_INITIALIZER(&mutex_tree_head);
RB_PROTOTYPE(mutex_tree, mutex, entry, _mutex_cmp)

struct cond {
	RB_ENTRY(cond)	 entry;
	int		 id;
	char		*name;
	time_t		 create_time;
	const char	*creator;
	unsigned int	 line;
	pthread_mutex_t  cond_mutex;
	pthread_cond_t	 sys_cond;
};
RB_HEAD(cond_tree, cond) cond_tree_head = RB_INITIALIZER(&cond_tree_head);
RB_PROTOTYPE(cond_tree, cond, entry, _cond_cmp)

struct rwlock {
	RB_ENTRY(rwlock) entry;
	int		 id;
	char		*name;
	time_t		 create_time;
	const char	*creator;
	unsigned int	 line;
	pthread_rwlock_t sys_rwlock;
};
RB_HEAD(rwlock_tree, rwlock) rwlock_tree_head = RB_INITIALIZER(&rwlock_tree_head);
RB_PROTOTYPE(rwlock_tree, rwlock, entry, _rwlock_cmp)


static int		 thread_initialized = 0;
static int		 next_thread_id = 0;
static int		 next_mutex_id = 0;
static int		 next_cond_id = 0;
static int		 next_rwlock_id = 0;
static unsigned int	 debug_level = 0;
static FILE		*debug_output = NULL;
static struct mutex	*threadtree_mutex;
static struct mutex	*mutextree_mutex;
static struct mutex	*condtree_mutex;
static struct mutex	*rwlocktree_mutex;
static struct mutex	*library_mutex;
static const char	*unknown_file = "<unknown>";


RB_GENERATE(thread_tree, thread, entry, _thread_cmp)
RB_GENERATE(mutex_tree, mutex, entry, _mutex_cmp)
RB_GENERATE(cond_tree, cond, entry, _cond_cmp)
RB_GENERATE(rwlock_tree, rwlock, entry, _rwlock_cmp)

void
_thread_fatal(int errnum, const char *fmt, ...)
{
#ifndef THREAD_DIE_SILENT
	va_list ap;

	if (debug_output == NULL)
		debug_output = THREAD_DEFAULT_OUTPUT;

	if (thread_initialized)
		THREAD_PROTECT(
			       {
				       va_start(ap, fmt);
				       vfprintf(debug_output, fmt, ap);
				       va_end(ap);
				       if (errnum > 0)
					       fprintf(debug_output, ": %s\n",
						       strerror(errnum));
				       fflush(debug_output);
			       });
	else {
		va_start(ap, fmt);
		vfprintf(debug_output, fmt, ap);
		va_end(ap);
		if (errnum > 0)
			fprintf(debug_output, ": %s\n", strerror(errnum));
		fflush(debug_output);
	}
#endif /* !THREAD_DIE_SILENT */
	abort();
}

void
_thread_debug_printf(unsigned int level, const char *fmt, ...)
{
	va_list ap;

	if (level > debug_level)
		return;

	va_start(ap, fmt);
#ifdef THREAD_DEBUG
	if (thread_initialized)
		THREAD_PROTECT(
			       {
				       vfprintf(debug_output, fmt, ap);
				       fflush(debug_output);
			       });
	else {
		vfprintf(debug_output, fmt, ap);
		fflush(debug_output);
	}
#endif /* THREAD_DEBUG */
	va_end(ap);
}

void
_init_mutexes(void)
{
	int	m_id = 0;

	mutextree_mutex = xcalloc(1, sizeof(struct mutex));
	mutextree_mutex->id = --m_id;
	mutextree_mutex->name = xstrdup("Mutex-Tree Mutex");
	mutextree_mutex->state = MUTEX_NEVERLOCKED;
	mutextree_mutex->create_time = time(NULL);
	mutextree_mutex->creator = __FILE__;
	mutextree_mutex->line = __LINE__;
	_mutex_init(mutextree_mutex);

	threadtree_mutex = xcalloc(1, sizeof(struct mutex));
	threadtree_mutex->id = --m_id;
	threadtree_mutex->name = xstrdup("Thread-Tree Mutex");
	threadtree_mutex->state = MUTEX_NEVERLOCKED;
	threadtree_mutex->create_time = time(NULL);
	threadtree_mutex->creator = __FILE__;
	threadtree_mutex->line = __LINE__;
	_mutex_init(threadtree_mutex);

	condtree_mutex = xcalloc(1, sizeof(struct mutex));
	condtree_mutex->id = --m_id;
	condtree_mutex->name = xstrdup("Cond-Tree Mutex");
	condtree_mutex->state = MUTEX_NEVERLOCKED;
	condtree_mutex->create_time = time(NULL);
	condtree_mutex->creator = __FILE__;
	condtree_mutex->line = __LINE__;
	_mutex_init(condtree_mutex);

	rwlocktree_mutex = xcalloc(1, sizeof(struct mutex));
	rwlocktree_mutex->id = --m_id;
	rwlocktree_mutex->name = xstrdup("RWLock-Tree Mutex");
	rwlocktree_mutex->state = MUTEX_NEVERLOCKED;
	rwlocktree_mutex->create_time = time(NULL);
	rwlocktree_mutex->creator = __FILE__;
	rwlocktree_mutex->line = __LINE__;
	_mutex_init(rwlocktree_mutex);

	library_mutex = xcalloc(1, sizeof(struct mutex));
	library_mutex->id = --m_id;
	library_mutex->name = xstrdup("Library Call Mutex");
	library_mutex->state = MUTEX_NEVERLOCKED;
	library_mutex->create_time = time(NULL);
	library_mutex->creator = __FILE__;
	library_mutex->line = __LINE__;
	_mutex_init(library_mutex);
}

void
_catch_signals(void)
{
#ifndef WIN32
	sigset_t	ss;
	int		error;

	sigemptyset(&ss);

	/*
	 * These signals should only be accepted by the signal handling thread
	 * (main thread.)
	 */
	sigaddset(&ss, SIGCHLD);
	sigaddset(&ss, SIGHUP);
	sigaddset(&ss, SIGINT);
	sigaddset(&ss, SIGPIPE);
	sigaddset(&ss, SIGTERM);
	sigaddset(&ss, SIGUSR1);
	sigaddset(&ss, SIGUSR2);

	if ((error = pthread_sigmask(SIG_UNBLOCK, &ss, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_sigmask()");
#endif /* !WIN32 */
}

void
_block_signals(void)
{
#ifndef WIN32
	sigset_t	ss;
	int		error;

	sigfillset(&ss);

	/*
	 * These are the only signals that sub-threads may receive.
	 */
	sigdelset(&ss, SIGBUS);
	sigdelset(&ss, SIGFPE);
	sigdelset(&ss, SIGKILL);
	sigdelset(&ss, SIGSEGV);
	sigdelset(&ss, SIGSTOP);
	if ((error = pthread_sigmask(SIG_BLOCK, &ss, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_sigmask()");
#endif /* !WIN32 */
}

int
_thread_cmp(void *arg_a, void *arg_b)
{
	struct thread	*a = (struct thread *)arg_a;
	struct thread	*b = (struct thread *)arg_b;

	return (a->id - b->id);
}

int
_mutex_cmp(void *arg_a, void *arg_b)
{
	struct mutex	*a = (struct mutex *)arg_a;
	struct mutex	*b = (struct mutex *)arg_b;

	return (a->id - b->id);
}

int
_cond_cmp(void *arg_a, void *arg_b)
{
	struct cond	*a = (struct cond *)arg_a;
	struct cond	*b = (struct cond *)arg_b;

	return (a->id - b->id);
}

int
_rwlock_cmp(void *arg_a, void *arg_b)
{
	struct rwlock	*a = (struct rwlock *)arg_a;
	struct rwlock	*b = (struct rwlock *)arg_b;

	return (a->id - b->id);
}

void
_thread_free(struct thread **thread_p)
{
	struct thread	*thread = *thread_p;

	if (thread->name != NULL)
		xfree(thread->name);
	if (thread->creator != NULL)
		thread->creator = NULL;
	xfree(*thread_p);
}

void *
_thread_start_routine(void *arg)
{
	struct thread_start	*start = (struct thread_start *)arg;
	void *			 (*start_routine)(void *) = start->start_routine;
	void			*real_arg = start->start_arg;
	struct thread		*thread = start->thread;
	void			*ret;
	int			 error;

	_block_signals();

	_mutex_lock(threadtree_mutex);
	thread->sys_thread = pthread_self();
	RB_INSERT(thread_tree, &thread_tree_head, thread);
	_mutex_unlock(threadtree_mutex);

	if ((error = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_setcancelstate()");

	xfree(start);

	ret = (start_routine)(real_arg);

	if (thread->detached) {
		_mutex_lock(threadtree_mutex);
		RB_REMOVE(thread_tree, &thread_tree_head, thread);
		_thread_free(&thread);
		_mutex_unlock(threadtree_mutex);
	}

	return (ret);
}

void
_mutex_init(struct mutex *mutex)
{
	int	error;

	if ((error = pthread_mutex_init(&mutex->sys_mutex, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_init()");
}

void
_mutex_lock(struct mutex *mutex)
{
	int	error;

	if ((error = pthread_mutex_lock(&mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_lock()");
	mutex->state = MUTEX_LOCKED;
}

void
_mutex_unlock(struct mutex *mutex)
{
	int	error;

	if ((error = pthread_mutex_unlock(&mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_unlock()");
	mutex->state = MUTEX_UNLOCKED;
}

void
_mutex_free(struct mutex **mutex_p)
{
	struct mutex	*mutex = *mutex_p;

	if (mutex->name != NULL)
		xfree(mutex->name);
	if (mutex->creator != NULL)
		mutex->creator = NULL;
	xfree(*mutex_p);
}

void
_cond_free(struct cond **cond_p)
{
	struct cond	*cond = *cond_p;

	if (cond->name != NULL)
		xfree(cond->name);
	if (cond->creator != NULL)
		cond->creator = NULL;
	xfree(*cond_p);
}

void
_rwlock_free(struct rwlock **rwlock_p)
{
	struct rwlock	*rwlock = *rwlock_p;

	if (rwlock->name != NULL)
		xfree(rwlock->name);
	if (rwlock->creator != NULL)
		rwlock->creator = NULL;
	xfree(*rwlock_p);
}


void
thread_initialize_debug(unsigned int level, FILE *output)
{
	struct thread	*thread;

	if (thread_initialized)
		_thread_fatal(0, "THREAD: Internal error: thread_initialize(): Thread library already initialized\n");

	if ((debug_level = level) > THREAD_DBGLVL_MAX)
		debug_level = THREAD_DBGLVL_MAX;
	if (output == NULL)
		debug_output = THREAD_DEFAULT_OUTPUT;
	else
		debug_output = output;

	_init_mutexes();

	_thread_debug_printf(3, "======> INIT thread\n");

	thread = xcalloc(1, sizeof(struct thread));
	thread->id = next_thread_id++;
	thread->name = xstrdup("Main Thread");
	thread->create_time = time(NULL);
	thread->creator = __FILE__;
	thread->line = __LINE__;
	thread->sys_thread = pthread_self();
	RB_INSERT(thread_tree, &thread_tree_head, thread);

	_catch_signals();

	thread_initialized = 1;

	_thread_debug_printf(4, "======> INIT thread done\n");
}

void
thread_shutdown(void)
{
	struct thread	*thread, *thread_next;
	struct mutex	*mutex, *mutex_next;
	struct cond	*cond, *cond_next;
	struct rwlock	*rwlock, *rwlock_next;
	int		 error;

	_thread_debug_printf(3, "======> SHUTDOWN thread\n");

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_shutdown(): Thread library not initialized\n");

	thread_initialized = 0;

	_mutex_lock(threadtree_mutex);
	_mutex_lock(mutextree_mutex);
	_mutex_lock(condtree_mutex);
	_mutex_lock(rwlocktree_mutex);

	for (rwlock = RB_MIN(rwlock_tree, &rwlock_tree_head);
	     rwlock != NULL;
	     rwlock = rwlock_next) {
		rwlock_next = RB_NEXT(rwlock_tree, &rwlock_tree_head, rwlock);
		RB_REMOVE(rwlock_tree, &rwlock_tree_head, rwlock);
		thread_rwlock_destroy(&rwlock);
	}

	for (cond = RB_MIN(cond_tree, &cond_tree_head);
	     cond != NULL;
	     cond = cond_next) {
		cond_next = RB_NEXT(cond_tree, &cond_tree_head, cond);
		RB_REMOVE(cond_tree, &cond_tree_head, cond);
		thread_cond_destroy(&cond);
	}

	for (mutex = RB_MIN(mutex_tree, &mutex_tree_head);
	     mutex != NULL;
	     mutex = mutex_next) {
		mutex_next = RB_NEXT(mutex_tree, &mutex_tree_head, mutex);
		RB_REMOVE(mutex_tree, &mutex_tree_head, mutex);
		thread_mutex_destroy(&mutex);
	}

	for (thread = RB_MIN(thread_tree, &thread_tree_head);
	     thread != NULL;
	     thread = thread_next) {
		thread_next = RB_NEXT(thread_tree, &thread_tree_head, thread);
		RB_REMOVE(thread_tree, &thread_tree_head, thread);
		_thread_free(&thread);
	}

	_mutex_unlock(rwlocktree_mutex);
	_mutex_unlock(condtree_mutex);
	_mutex_unlock(mutextree_mutex);
	_mutex_unlock(threadtree_mutex);

	if ((error = pthread_mutex_destroy(&library_mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy(%s)",
			      library_mutex->name);
	_mutex_free(&library_mutex);
	if ((error = pthread_mutex_destroy(&rwlocktree_mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy(%s)",
			      rwlocktree_mutex->name);
	_mutex_free(&rwlocktree_mutex);
	if ((error = pthread_mutex_destroy(&condtree_mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy(%s)",
			      condtree_mutex->name);
	_mutex_free(&condtree_mutex);
	if ((error = pthread_mutex_destroy(&threadtree_mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy(%s)",
			      threadtree_mutex->name);
	_mutex_free(&threadtree_mutex);
	if ((error = pthread_mutex_destroy(&mutextree_mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy(%s)",
			      mutextree_mutex->name);
	_mutex_free(&mutextree_mutex);

	_thread_debug_printf(4, "======> SHUTDOWN thread done\n");
}

struct thread *
thread_create_c(const char *name, void *(*start_routine)(void *),
		void *start_routine_arg, int detached, unsigned int stack,
		const char *file, unsigned int line)
{
	struct thread		*thread = NULL;
	struct thread_start	*thread_start;
	pthread_attr_t		 attr;
	int			 error;
	size_t			 stacksize;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_create(): Thread library not initialized\n");

	_thread_debug_printf(3, "======> CREATE THREAD: %s:%u: '%s' (%stached)\n",
			     file ? file : unknown_file, line,
			     name, detached ? "de" : "at");

	if (stack > 0)
		stacksize = stack;
	else
		stacksize = THREAD_DEFAULT_STACKSIZE;

	thread = xcalloc(1, sizeof(struct thread));
	thread_start = xcalloc(1, sizeof(struct thread_start));
	if ((error = pthread_attr_init(&attr)) != 0)
		_thread_fatal(error, "THREAD: pthread_attr_init(%s)",
			      name);

	_mutex_lock(threadtree_mutex);
	thread->id = next_thread_id++;
	_mutex_unlock(threadtree_mutex);
	thread->name = xstrdup(name);
	thread->create_time = time(NULL);
	if (file)
		thread->creator = file;
	else
		thread->creator = unknown_file;
	thread->line = line;
	thread->detached = detached;

	thread_start->start_routine = start_routine;
	thread_start->start_arg = start_routine_arg;
	thread_start->thread = thread;

	if ((error = pthread_attr_setstacksize(&attr, stacksize)) != 0)
		_thread_fatal(error, "THREAD: pthread_attr_setstacksize(%s)",
			      name);
	if ((error = pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED)) != 0)
		_thread_fatal(error, "THREAD: pthread_attr_setinheritsched(%s)",
			      name);
	if (thread->detached) {
		if ((error = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
			_thread_fatal(error, "THREAD: pthread_attr_setdetachstate(%s)",
				      name);
	}

	if ((error = pthread_create(&thread->sys_thread, &attr,
				    _thread_start_routine, thread_start)) != 0)
		_thread_fatal(error, "THREAD: pthread_create(%s)",
			      name);

	if ((error = pthread_attr_destroy(&attr)) != 0)
		_thread_fatal(error, "THREAD: pthread_attr_destroy()");

	_thread_debug_printf(4, "======> CREATE THREAD done: '%s'(%d)\n",
			     thread->name, thread->id);

	return (thread);
}

struct thread *
thread_self_c(const char *file, unsigned int line)
{
	struct thread	*thread = NULL;
	pthread_t	 sys_thread;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_self(): Thread library not initialized\n");

	sys_thread = pthread_self();

	_mutex_lock(threadtree_mutex);
	RB_FOREACH(thread, thread_tree, &thread_tree_head) {
		if (thread && pthread_equal(sys_thread, thread->sys_thread))
			break;
	}
	_mutex_unlock(threadtree_mutex);

	if (thread == NULL)
		_thread_fatal(0, "THREAD: thread_self(): In %s:%u: No thread exist (anymore)\n",
			      file ? file : unknown_file, line);

	return (thread);
}

void
thread_rename(const char *name)
{
	struct thread	*thread;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_rename(): Thread library not initialized\n");

	thread = thread_self();
	if (thread->name != NULL)
		xfree(thread->name);
	thread->name = xstrdup(name);
}

void
thread_join_c(struct thread **thread_p, const char *file, unsigned int line)
{
	struct thread	*thread = NULL;
	int		 error;
	void		*ret;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_join(): Thread library not initialized\n");

	if (thread_p == NULL || (thread = *thread_p) == NULL || thread == NULL)
		_thread_fatal(0, "THREAD: thread_join(): Bad argument\n");

	_thread_debug_printf(3, "======> THREAD JOIN: %s:%u: '%s'(%d) (from %s:%u, age %ds)\n",
			     file ? file : unknown_file, line, thread->name,
			     thread->id, thread->creator, thread->line,
			     (int)(time(NULL) - thread->create_time));

	if ((error = pthread_join(thread->sys_thread, &ret)) != 0)
		_thread_fatal(error, "THREAD: pthread_join()");

	_mutex_lock(threadtree_mutex);
	RB_REMOVE(thread_tree, &thread_tree_head, thread);
	_thread_free(thread_p);
	_mutex_unlock(threadtree_mutex);

	_thread_debug_printf(4, "======> THREAD JOIN done\n");
}

void
thread_exit_c(int val, const char *file, unsigned int line)
{
	struct thread	*thread;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_exit(): Thread library not initialized\n");

	thread = thread_self();

	_thread_debug_printf(3, "======> THREAD EXIT: %s:%u: '%s'(%d) (from %s:%u, age %ds)\n",
			     file ? file : unknown_file, line, thread->name,
			     thread->id, thread->creator, thread->line,
			     (int)(time(NULL) - thread->create_time));

#ifdef THREAD_CHECK_MUTEXES
	if (debug_level > 0) {
		struct mutex	*mutex;

		_mutex_lock(mutextree_mutex);
		RB_FOREACH(mutex, mutex_tree, &mutex_tree_head) {
			if (mutex->state == MUTEX_LOCKED &&
			    mutex->thread_id == thread->id)
				_thread_debug_printf(1, "#> THREAD EXIT: Thread '%s'(%d) from %s:%u exiting in %s:%u without unlocking mutex '%s'(%d)\n",
						     thread->name, thread->id,
						     thread->creator,
						     thread->line,
						     file ? file : unknown_file,
						     line, mutex->name,
						     mutex->id);
		}
		_mutex_unlock(mutextree_mutex);
	}
#endif /* THREAD_CHECK_MUTEXES */

	if (thread->detached) {
		_mutex_lock(threadtree_mutex);
		RB_REMOVE(thread_tree, &thread_tree_head, thread);
		_thread_free(&thread);
		_mutex_unlock(threadtree_mutex);
	}

	_thread_debug_printf(4, "======> THREAD exiting\n");

	pthread_exit((void *)&val);
}

struct mutex *
thread_mutex_create_c(const char *name, const char *file, unsigned int line)
{
	struct mutex	*mutex = NULL;
	struct thread	*thread = NULL;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_mutex_create(): Thread library not initialized\n");

	thread = thread_self();
	_thread_debug_printf(3, "======> CREATE MUTEX: %s:%u: %s\n",
			     file ? file : unknown_file, line, name);

	mutex = xcalloc(1, sizeof(struct mutex));
	mutex->thread_id = thread->id;
	mutex->name = xstrdup(name);
	mutex->state = MUTEX_NEVERLOCKED;
	mutex->create_time = time(NULL);
	if (file)
		mutex->creator = file;
	else
		mutex->creator = unknown_file;
	mutex->line = line;
	_mutex_init(mutex);
	_mutex_lock(mutextree_mutex);
	mutex->id = next_mutex_id++;
	RB_INSERT(mutex_tree, &mutex_tree_head, mutex);
	_mutex_unlock(mutextree_mutex);

	_thread_debug_printf(4, "======> CREATE MUTEX done: '%s'(%d)\n",
			     mutex->name, mutex->id);

	return (mutex);
}

void
thread_mutex_lock_c(struct mutex *mutex, const char *file, unsigned int line)
{
	struct thread	*thread;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_mutex_lock(): Thread library not initialized\n");

	if (mutex == NULL)
		_thread_fatal(0, "THREAD: thread_mutex_lock(): Bad argument\n");

	thread = thread_self();
	_thread_debug_printf(3, "======> LOCK MUTEX: %s:%u: '%s'(%d) thread '%s'%d\n",
			     file ? file : unknown_file, line, mutex->name,
			     mutex->id, thread->name, thread->id);

#ifdef THREAD_CHECK_MUTEXES
	if (debug_level > 0) {
		struct mutex	*mtx;
		unsigned int	 locks = 0;

		_mutex_lock(mutextree_mutex);
		RB_FOREACH(mtx, mutex_tree, &mutex_tree_head) {
			if (mtx->state == MUTEX_LOCKED &&
			    mtx->thread_id == thread->id) {
				locks++;
				if (mtx->id == mutex->id)
					_thread_debug_printf(1, "#> LOCK MUTEX: DEADLOCK DETECTED! Thread '%s'(%d) locks mutex '%s'(%d) twice in %s:%u\n",
							     thread->name,
							     thread->id,
							     mutex->name,
							     mutex->id,
							     file ? file : unknown_file,
							     line);
			}
		}
		_mutex_unlock(mutextree_mutex);

		if (locks > 0)
			_thread_debug_printf(1, "#> LOCK MUTEX: Warning: Thread '%s'(%d) already has %u locks\n",
					     thread->name, thread->id, locks);
	}
#endif /* THREAD_CHECK_MUTEXES */

	_mutex_lock(mutex);
	mutex->thread_id = thread->id;

	_thread_debug_printf(4, "======> LOCK MUTEX done: '%s'(%d)\n",
			     mutex->name, mutex->id);
}

void
thread_mutex_unlock_c(struct mutex *mutex, const char *file, unsigned int line)
{
	struct thread	*thread;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_mutex_unlock(): Thread library not initialized\n");

	if (mutex == NULL)
		_thread_fatal(0, "THREAD: thread_mutex_unlock(): Bad argument\n");

	thread = thread_self();
	_thread_debug_printf(3, "======> UNLOCK MUTEX: %s:%u: '%s'(%d) thread '%s'(%d)\n",
			     file ? file : unknown_file, line, mutex->name,
			     mutex->id, thread->name, thread->id);

#ifdef THREAD_CHECK_MUTEXES
	if (debug_level > 0) {
		struct mutex	*mtx;

		_mutex_lock(mutextree_mutex);
		RB_FOREACH(mtx, mutex_tree, &mutex_tree_head) {
			if (mtx->state == MUTEX_LOCKED &&
			    mtx->thread_id != thread->id &&
			    mtx->id == mutex->id)
				_thread_debug_printf(1, "#> UNLOCK MUTEX: ILLEGAL UNLOCK! Thread '%s'(%d) tries to unlock mutex '%s'(%d) without owning it in %s:%u\n",
						     thread->name, thread->id,
						     mutex->name, mutex->id,
						     file ? file : unknown_file,
						     line);
		}
		_mutex_unlock(mutextree_mutex);
	}
#endif /* THREAD_CHECK_MUTEXES */

	_mutex_unlock(mutex);
	mutex->thread_id = thread->id;

	_thread_debug_printf(4, "======> UNLOCK MUTEX done: '%s'(%d)\n",
			     mutex->name, mutex->id);
}

void
thread_mutex_destroy_c(struct mutex **mutex_p,
		       const char *file, unsigned int line)
{
	struct mutex	*mutex = NULL;
	struct thread	*thread;
	int		 error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_mutex_destroy(): Thread library not initialized\n");

	if (mutex_p == NULL || (mutex = *mutex_p) == NULL || mutex == NULL)
		_thread_fatal(0, "THREAD: thread_mutex_destroy(): Bad argument\n");

	thread = thread_self();
	_thread_debug_printf(3, "======> MUTEX DESTROY: %s:%u: '%s'(%d) (from %s:%u, age %ds) thread '%s'(%d)\n",
			     file ? file : unknown_file, line, mutex->name,
			     mutex->id, mutex->creator, mutex->line,
			     (int)(time(NULL) - mutex->create_time),
			     thread->name, thread->id);

	if (mutex->state == MUTEX_NEVERLOCKED)
		_thread_debug_printf(2, ">>>>> Mutex '%s'(%d) from %s:%u was never locked\n",
				     mutex->name, mutex->id, mutex->creator,
				     mutex->line);

	if ((error = pthread_mutex_destroy(&mutex->sys_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy()");
	_mutex_lock(mutextree_mutex);
	RB_REMOVE(mutex_tree, &mutex_tree_head, mutex);
	_mutex_free(mutex_p);
	_mutex_unlock(mutextree_mutex);

	_thread_debug_printf(4, "======> MUTEX DESTROY done\n");
}

struct cond *
thread_cond_create_c(const char *name, const char *file, unsigned int line)
{
	struct cond	*cond = NULL;
	int		 error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_cond_create(): Thread library not initialized\n");

	_thread_debug_printf(3, "======> CREATE COND: %s:%u: %s\n",
			     file ? file : unknown_file, line, name);

	cond = xcalloc(1, sizeof(struct cond));
	cond->name = xstrdup(name);
	cond->create_time = time(NULL);
	if (file)
		cond->creator = file;
	else
		cond->creator = unknown_file;
	cond->line = line;
	if ((error = pthread_cond_init(&cond->sys_cond, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_cond_init()");
	if ((error = pthread_mutex_init(&cond->cond_mutex, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_init()");
	_mutex_lock(condtree_mutex);
	cond->id = next_cond_id++;
	RB_INSERT(cond_tree, &cond_tree_head, cond);
	_mutex_unlock(condtree_mutex);

	_thread_debug_printf(4, "======> CREATE COND done: '%s'(%d)\n",
			     cond->name, cond->id);

	return (cond);
}

void
thread_cond_signal_c(struct cond *cond, const char *file, unsigned int line)
{
	int	error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_cond_signal(): Thread library not initialized\n");

	if (cond == NULL)
		_thread_fatal(0, "THREAD: thread_cond_signal(): Bad argument\n");

	_thread_debug_printf(3, "======> SIGNAL COND: %s:%u: '%s'(%d)\n",
			     file ? file : unknown_file, line,
			     cond->name, cond->id);

	if ((error = pthread_cond_signal(&cond->sys_cond)) != 0)
		_thread_fatal(error, "THREAD: pthread_cond_signal()");
}

void
thread_cond_broadcast_c(struct cond *cond, const char *file, unsigned int line)
{
	int	error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_cond_broadcast(): Thread library not initialized\n");

	if (cond == NULL)
		_thread_fatal(0, "THREAD: thread_cond_broadcast(): Bad argument\n");

	_thread_debug_printf(3, "======> BROADCAST COND: %s:%u: '%s'(%d)\n",
			     file ? file : unknown_file, line,
			     cond->name, cond->id);

	if ((error = pthread_cond_broadcast(&cond->sys_cond)) != 0)
		_thread_fatal(error, "THREAD: pthread_cond_broadcast()");
}

void
thread_cond_wait_c(struct cond *cond, const char *file, unsigned int line)
{
	int	error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_cond_wait(): Thread library not initialized\n");

	if (cond == NULL)
		_thread_fatal(0, "THREAD: thread_cond_wait(): Bad argument\n");

	_thread_debug_printf(3, "======> COND WAIT: %s:%u: '%s'(%d)\n",
			     file ? file : unknown_file, line,
			     cond->name, cond->id);

	if ((error = pthread_mutex_lock(&cond->cond_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_lock()");
	if ((error = pthread_cond_wait(&cond->sys_cond,
				       &cond->cond_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_cond_wait()");
	if ((error = pthread_mutex_unlock(&cond->cond_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_unlock()");

	_thread_debug_printf(4, "======> COND WAIT done: '%s'(%d)\n",
			     cond->name, cond->id);
}

void
thread_cond_timedwait_c(struct cond *cond, unsigned int ms,
			const char *file, unsigned int line)
{
	int		error;
	struct timespec tv;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_cond_timedwait(): Thread library not initialized\n");

	if (cond == NULL)
		_thread_fatal(0, "THREAD: thread_cond_timedwait(): Bad argument\n");

	_thread_debug_printf(3, "======> COND TIMEDWAIT (%ums): %s:%u: '%s'(%d)\n",
			     ms, file ? file : unknown_file, line,
			     cond->name, cond->id);

	tv.tv_sec = ms / 1000;
	tv.tv_nsec = (ms - tv.tv_sec * 1000) * 1000000;

	if ((error = pthread_mutex_lock(&cond->cond_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_lock()");
	if ((error = pthread_cond_timedwait(&cond->sys_cond, &cond->cond_mutex,
					    &tv)) != 0)
		_thread_fatal(error, "THREAD: pthread_cond_wait()");
	if ((error = pthread_mutex_unlock(&cond->cond_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_unlock()");

	_thread_debug_printf(4, "======> COND TIMEDWAIT (%ums) done: '%s'(%d)\n",
			     ms, cond->name, cond->id);
}

void
thread_cond_destroy_c(struct cond **cond_p,
		      const char *file, unsigned int line)
{
	int		 error;
	struct cond	*cond = NULL;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_cond_destroy(): Thread library not initialized\n");

	if (cond_p == NULL || (cond = *cond_p) == NULL || cond == NULL)
		_thread_fatal(0, "THREAD: thread_cond_destroy(): Bad argument\n");

	_thread_debug_printf(3, "======> COND DESTROY: %s:%u: '%s'(%d) (from %s:%u, age %ds)\n",
			     file ? file : unknown_file, line, cond->name,
			     cond->id, cond->creator, cond->line,
			     (int)(time(NULL) - cond->create_time));

	if ((error = pthread_mutex_destroy(&cond->cond_mutex)) != 0)
		_thread_fatal(error, "THREAD: pthread_mutex_destroy()");
	if ((error = pthread_cond_destroy(&cond->sys_cond)) != 0)
		_thread_fatal(error, "THREAD: pthread_cond_destroy()");
	_mutex_lock(condtree_mutex);
	RB_REMOVE(cond_tree, &cond_tree_head, cond);
	_cond_free(cond_p);
	_mutex_unlock(condtree_mutex);

	_thread_debug_printf(4, "======> COND DESTROY done\n");
}

struct rwlock *
thread_rwlock_create_c(const char *name, const char *file, unsigned int line)
{
	struct rwlock	*rwlock = NULL;
	int		 error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_rwlock_create(): Thread library not initialized\n");

	_thread_debug_printf(3, "======> CREATE RWLOCK: %s:%u: %s\n",
			     file ? file : unknown_file, line, name);

	rwlock = xcalloc(1, sizeof(struct rwlock));
	rwlock->name = xstrdup(name);
	rwlock->create_time = time(NULL);
	if (file)
		rwlock->creator = file;
	else
		rwlock->creator = unknown_file;
	rwlock->line = line;
	if ((error = pthread_rwlock_init(&rwlock->sys_rwlock, NULL)) != 0)
		_thread_fatal(error, "THREAD: pthread_rwlock_init()");
	_mutex_lock(rwlocktree_mutex);
	rwlock->id = next_rwlock_id++;
	RB_INSERT(rwlock_tree, &rwlock_tree_head, rwlock);
	_mutex_unlock(rwlocktree_mutex);

	_thread_debug_printf(4, "======> CREATE RWLOCK done: '%s'(%d)\n",
			     rwlock->name, rwlock->id);

	return (rwlock);
}

void
thread_rwlock_rlock_c(struct rwlock *rwlock,
		      const char *file, unsigned int line)
{
	int	error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_rwlock_rlock(): Thread library not initialized\n");

	if (rwlock == NULL)
		_thread_fatal(0, "THREAD: thread_rwlock_rlock(): Bad argument\n");

	_thread_debug_printf(3, "======> RWLOCK READ: %s:%u: '%s'(%d)\n",
			     file ? file : unknown_file, line,
			     rwlock->name, rwlock->id);

	if ((error = pthread_rwlock_rdlock(&rwlock->sys_rwlock)) != 0)
		_thread_fatal(error, "THREAD: pthread_rwlock_rdlock()");
}

void
thread_rwlock_wlock_c(struct rwlock *rwlock,
		      const char *file, unsigned int line)
{
	int	error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_rwlock_wlock(): Thread library not initialized\n");

	if (rwlock == NULL)
		_thread_fatal(0, "THREAD: thread_rwlock_wlock(): Bad argument\n");

	_thread_debug_printf(3, "======> RWLOCK WRITE: %s:%u: '%s'(%d)\n",
			     file ? file : unknown_file, line,
			     rwlock->name, rwlock->id);

	if ((error = pthread_rwlock_wrlock(&rwlock->sys_rwlock)) != 0)
		_thread_fatal(error, "THREAD: pthread_rwlock_wrlock()");
}

void
thread_rwlock_unlock_c(struct rwlock *rwlock,
		       const char *file, unsigned int line)
{
	int	error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_rwlock_unlock(): Thread library not initialized\n");

	if (rwlock == NULL)
		_thread_fatal(0, "THREAD: thread_rwlock_unlock(): Bad argument\n");

	_thread_debug_printf(3, "======> RWLOCK UNLOCK: %s:%u: '%s'(%d)\n",
			     file ? file : unknown_file, line,
			     rwlock->name, rwlock->id);

	if ((error = pthread_rwlock_unlock(&rwlock->sys_rwlock)) != 0)
		_thread_fatal(error, "THREAD: pthread_rwlock_unlock()");
}

void
thread_rwlock_destroy_c(struct rwlock **rwlock_p,
			const char *file, unsigned int line)
{
	struct rwlock	*rwlock = NULL;
	int		 error;

	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_rwlock_destroy(): Thread library not initialized\n");

	if (rwlock_p == NULL || (rwlock = *rwlock_p) == NULL || rwlock == NULL)
		_thread_fatal(0, "THREAD: thread_rwlock_destroy(): Bad argument\n");

	_thread_debug_printf(3, "======> RWLOCK DESTROY: %s:%u: '%s'(%d) (from %s:%u, age %ds)\n",
			     file ? file : unknown_file, line, rwlock->name,
			     rwlock->id, rwlock->creator, rwlock->line,
			     (int)(time(NULL) - rwlock->create_time));

	if ((error = pthread_rwlock_destroy(&rwlock->sys_rwlock)) != 0)
		_thread_fatal(error, "THREAD: pthread_rwlock_destroy()");
	_mutex_lock(rwlocktree_mutex);
	RB_REMOVE(rwlock_tree, &rwlock_tree_head, rwlock);
	_rwlock_free(rwlock_p);
	_mutex_unlock(rwlocktree_mutex);

	_thread_debug_printf(4, "======> RWLOCK DESTROY done\n");
}

void
thread_sleep_ms(unsigned int ms)
{
	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_sleep(): Thread library not initialized\n");

#ifdef WIN32
	Sleep(ms);
#else
# ifdef HAVE_NANOSLEEP
	{
		struct timespec tv_sleep;
		struct timespec tv_remaining;
		int ret;

		tv_sleep.tv_sec = ms / 1000;
		tv_sleep.tv_nsec = (ms % 1000) * 1000000;

		while ((ret = nanosleep(&tv_sleep, &tv_remaining)) != 0 &&
		       errno == EINTR) {
			tv_sleep.tv_sec = tv_remaining.tv_sec;
			tv_sleep.tv_nsec = tv_remaining.tv_nsec;
		}
	}
# else
	poll(NULL, 0, ms);
# endif /* HAVE_NANOSLEEP */
#endif /* WIN32 */
}

void
thread_library_lock(void)
{
	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_library_lock(): Thread library not initialized\n");

	_mutex_lock(library_mutex);
}

void
thread_library_unlock(void)
{
	if (!thread_initialized)
		_thread_fatal(0, "THREAD: thread_library_unlock(): Thread library not initialized\n");

	_mutex_unlock(library_mutex);
}
