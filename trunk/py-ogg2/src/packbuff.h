#ifndef PYOGGPACKBUFF_H
#define PYOGGPACKBUFF_H

#include "packet.h"

typedef struct {
  PyObject_HEAD
  int msb_flag; /* 0 = LSb (standard), 1 = MSb */
  int write_flag; /* 0 = read, 1 = write, 2 = dead write */
  oggpack_buffer *buffer;
  PyOggPacketObject *packetobj;  /* temporary workaround */
} PyOggPackBufferObject;

#define PyOggPackBuffer_AsOggPackBuffer(x) ( ((PyOggPackBufferObject *) (x))->buffer )

extern PyTypeObject PyOggPackBuffer_Type;

PyObject *PyOggPackBuffer_New(PyObject *, PyObject *);
PyObject *PyOggPackBuffer_NewB(PyObject *, PyObject *);

#endif
