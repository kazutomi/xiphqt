#ifndef PYOGGPACKBUFF_H
#define PYOGGPACKBUFF_H

#include <pyogg/pyogg2.h>

typedef struct {
  PyObject_HEAD
  oggpack_buffer *buffer;
} PyOggPackBufferObject;

#define PyOggPackBuffer_AsOggPackBuffer(x) ( ((PyOggPackBufferObject *) (x))->buffer )

extern PyTypeObject PyOggPackBuffer_Type;

PyObject *PyOggPackBuffer_New(PyObject *, PyObject *);

#endif
