#ifndef PYOGGSYNCSTATE_H
#define PYOGGSYNCSTATE_H

#include <Python.h>
#include <ogg/ogg.h>

typedef struct {
  PyObject_HEAD
  ogg_sync_state os;
} py_ogg_sync_state;


extern PyTypeObject py_ogg_sync_state_type;

#endif
