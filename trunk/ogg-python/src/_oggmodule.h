#ifndef __OGGMODULE_H__
#define __OGGMODULE_H__

#include <Python.h>

PyObject *Py_OggError;

/* Object docstrings */

extern char py_ogg_stream_state_doc[];
extern char py_oggpack_buffer_doc[];
extern char py_ogg_sync_state_doc[];

#endif /* __OGGMODULE_H__ */
