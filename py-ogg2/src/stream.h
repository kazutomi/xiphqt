#ifndef PYOGGSTREAMSTATE_H
#define PYOGGSTREAMSTATE_H

#include "general.h"

typedef struct {
  PyObject_HEAD
  ogg2_stream_state *stream;
} PyOggStreamStateObject;

extern PyTypeObject PyOggStreamState_Type;

#define PyOggStreamState_AsOggStreamState(x) (((PyOggStreamStateObject *) (x))->stream)

PyObject *PyOggStreamState_FromSerialno(int);
PyObject *PyOggStreamState_New(PyObject *, PyObject *);

#endif
