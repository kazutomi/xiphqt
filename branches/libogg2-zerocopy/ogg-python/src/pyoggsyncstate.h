#ifndef PYOGGSYNCSTATE_H
#define PYOGGSYNCSTATE_H

#include <Python.h>
#include <ogg/ogg.h>

typedef struct {
  PyObject_HEAD
  ogg_sync_state os;
} py_ogg_sync_state;

#define PY_OGG_SYNC_STATE(x) (&(((py_ogg_sync_state *) (x))->os))

extern PyTypeObject py_ogg_sync_state_type;

PyObject *py_ogg_sync_state_new(PyObject *, PyObject *);

#endif
