#ifndef PYOGGPACKBUFF_H
#define PYOGGPACKBUFF_H

#include <ogg/ogg.h>
#include <Python.h>

typedef struct {
  PyObject_HEAD
  oggpack_buffer ob;
} py_oggpack_buffer;

#define PY_OGGPACK_BUFF(x) (&(((py_oggpack_buffer *) (x))->ob))

extern PyTypeObject py_oggpack_buffer_type;

PyObject *py_oggpack_buffer_new(PyObject *, PyObject *);

#endif
