#ifndef PYOGGPACKET_H
#define PYOGGPACKET_H

#include <ogg/ogg.h>
#include <Python.h>

typedef struct {
  PyObject_HEAD
  ogg_packet op;
} py_ogg_packet;

extern PyTypeObject py_ogg_packet_type;

#define PY_OGG_PACKET(x) (&(((py_ogg_packet *) (x))->op))

PyObject *py_ogg_packet_from_packet(ogg_packet *);

#endif /* PYOGGPACKET_H */
