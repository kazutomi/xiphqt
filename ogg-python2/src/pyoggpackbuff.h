#ifndef PYOGGPACKBUFF_H
#define PYOGGPACKBUFF_H

#include <pyogg/pyogg2.h>
#include "pyoggpacket.h"

typedef struct {
  PyObject_HEAD
  int write_flag; /* 0 = read, 1 = write, 2 = dead write */
  oggpack_buffer *buffer;
  PyOggPacketObject *packetobj;  /* temporary workaround */
} PyOggPackBufferObject;

#define PyOggPackBuffer_AsOggPackBuffer(x) ( ((PyOggPackBufferObject *) (x))->buffer )

extern PyTypeObject PyOggPackBuffer_Type;

PyObject *PyOggPackBuffer_New(PyObject *, PyObject *);

#endif
