#ifndef PYOGGSTREAMSTATE_H
#define PYOGGSTREAMSTATE_H

#include <ogg/ogg.h>
#include <Python.h>

typedef struct {
  PyObject_HEAD
  ogg_stream_state os;
} py_ogg_stream_state;


extern PyTypeObject py_ogg_stream_state_type;

#define PY_OGG_STREAM(x) (&(((py_ogg_stream_state *) (x))->os))

PyObject *py_ogg_stream_state_from_serialno(int);
PyObject *py_ogg_stream_state_new(PyObject *, PyObject *);

#endif
