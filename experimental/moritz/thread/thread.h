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

#ifndef __THREAD_H__
#define __THREAD_H__

/*
 * Define THREAD_DEBUG to get the thread_debug_initialize_debug() function,
 * which enables debugging output of /level/s between 0 and 4, inclusive.
 * The higher the number, the more verbose is the output. A value of 0 is
 * equivalent to using thread_initialize().
 * Define THREAD_CHECK_MUTEXES to perform additional checks on mutexes.
 * To make libthread abort() silently and never output anything, define
 * THREAD_DIE_SILENT (will be undefined if THREAD_DEBUG is defined.)
 *
 * Note that none of the x*_c() functions should be used directly, unless it
 * is ensured that /file/ is a const char * to a real string constant.
 */
/* #define THREAD_DEBUG		1 */
/* #define THREAD_CHECK_MUTEXES	1 */
/* #define THREAD_DIE_SILENT	1 */


#define THREAD_DEFAULT_STACKSIZE (512 * 1024)
#define THREAD_DEFAULT_OUTPUT	stderr

#define THREAD_ATTACHED 	0
#define THREAD_DETACHED 	1


typedef struct thread	*thread_t;
typedef struct mutex	*mutex_t;
typedef struct cond	*cond_t;
typedef struct rwlock	*rwlock_t;


/*
 * Library initialization and shutdown.
 */

#define thread_initialize()					\
	thread_initialize_debug(0, NULL)
void		thread_initialize_debug(unsigned int /* level */,
					FILE * /* output stream */);
void		thread_shutdown(void);


/*
 * Threads
 */

#define thread_create(n, r, a, d)				\
	thread_create_c(n, r, a, d, 0, __FILE__, __LINE__)
#define thread_create_with_stacksize(n, r, a, d, s)		\
	thread_create_c(n, r, a, d, s, __FILE__, __LINE__)
thread_t	thread_create_c(const char * /* name */,
				void *(*)(void *) /* start routine */,
				void * /* start routine argument */,
				int /* run detached */,
				unsigned int /* stack size, 0 for default */,
				const char * /* file */,
				unsigned int /* line */);

#define thread_self()						\
	thread_self_c(__FILE__, __LINE__)
thread_t	thread_self_c(const char * /* file */,
			      unsigned int /* line */);

void		thread_rename(const char * /* name */);

#define thread_join(t)						\
	thread_join_c(t, __FILE__, __LINE__)
void		thread_join_c(thread_t * /* thread pointer */,
			      const char * /* file */,
			      unsigned int /* line */);

#define thread_exit(t)						\
	thread_exit_c(t, __FILE__, __LINE__)
void		thread_exit_c(int /* exit value */,
			      const char * /* file */,
			      unsigned int /* line */);


/*
 * Mutexes
 */

#define thread_mutex_create(n)					\
	thread_mutex_create_c(n, __FILE__, __LINE__)
mutex_t 	thread_mutex_create_c(const char * /* name */,
				      const char * /* file */,
				      unsigned int /* line */);

#define thread_mutex_lock(x)					\
	thread_mutex_lock_c(x, __FILE__, __LINE__)
void		thread_mutex_lock_c(mutex_t /* mutex */,
				    const char * /* file */,
				    unsigned int /* line */);

#define thread_mutex_unlock(x)					\
	thread_mutex_unlock_c(x, __FILE__, __LINE__)
void		thread_mutex_unlock_c(mutex_t /* mutex */,
				      const char * /* file */,
				      unsigned int /* line */);

#define thread_mutex_destroy(x) 				\
	thread_mutex_destroy_c(x, __FILE__, __LINE__)
void		thread_mutex_destroy_c(mutex_t * /* mutex pointer */,
				       const char * /* file */,
				       unsigned int /* line */);


/*
 * Conds
 */

#define thread_cond_create(n)					\
	thread_cond_create_c(n, __FILE__, __LINE__)
cond_t		thread_cond_create_c(const char * /* name */,
				     const char * /* file */,
				     unsigned int /* line */);

#define thread_cond_signal(c)					\
	thread_cond_signal_c(c, __FILE__, __LINE__)
void		thread_cond_signal_c(cond_t /* cond */,
				     const char * /* file */,
				     unsigned int /* line */);

#define thread_cond_broadcast(c)				\
	thread_cond_broadcast_c(c, __FILE__, __LINE__)
void		thread_cond_broadcast_c(cond_t /* cond */,
					const char * /* file */,
					unsigned int /* line */);

#define thread_cond_wait(c)					\
	thread_cond_wait_c(c, __FILE__, __LINE__)
void		thread_cond_wait_c(cond_t /* cond */,
				   const char * /* file */,
				   unsigned int /* line */);

#define thread_cond_timedwait(c, s)				\
	thread_cond_timedwait_c(c, s, __FILE__, __LINE__)
void		thread_cond_timedwait_c(cond_t /* cond */,
					unsigned int /* milliseconds */,
					const char * /* file */,
					unsigned int /* line */);

#define thread_cond_destroy(c)					\
	thread_cond_destroy_c(c, __FILE__, __LINE__)
void		thread_cond_destroy_c(cond_t * /* cond pointer */,
				      const char * /* file */,
				      unsigned int /* line */);


/*
 * RW locks
 */

#define thread_rwlock_create(n)					\
	thread_rwlock_create_c(n, __FILE__, __LINE__)
rwlock_t	thread_rwlock_create_c(const char * /* name */,
				       const char * /* file */,
				       unsigned int /* line */);

#define thread_rwlock_rlock(l)					\
	thread_rwlock_rlock_c(l, __FILE__, __LINE__)
void		thread_rwlock_rlock_c(rwlock_t /* rwlock */,
				      const char * /* file */,
				      unsigned int /* line */);

#define thread_rwlock_wlock(l)					\
	thread_rwlock_wlock_c(l, __FILE__, __LINE__)
void		thread_rwlock_wlock_c(rwlock_t /* rwlock */,
				      const char * /* file */,
				      unsigned int /* line */);

#define thread_rwlock_unlock(l) 				\
	thread_rwlock_unlock_c(l, __FILE__, __LINE__)
void		thread_rwlock_unlock_c(rwlock_t /* rwlock */,
				       const char * /* file */,
				       unsigned int /* line */);

#define thread_rwlock_destroy(l)				\
	thread_rwlock_destroy_c(l, __FILE__, __LINE__)
void		thread_rwlock_destroy_c(rwlock_t * /* rwlock pointer */,
					const char * /* file */,
					unsigned int /* line */);


/*
 * Miscellaneous
 */

/* The thread_sleep() macro takes microseconds for backwards-compatibility. */
#define thread_sleep(t) 					\
	thread_sleep_ms(t / 1000)
void		thread_sleep_ms(unsigned int /* milliseconds */);

void		thread_library_lock(void);
void		thread_library_unlock(void);
#define THREAD_PROTECT(code)			\
	do {					\
		thread_library_lock();		\
		code;				\
		thread_library_unlock();	\
	} while (0)

#endif /* __THREAD_H__ */
