#ifndef PYOGGPACKET_H
#define PYOGGPACKET_H

typedef struct {
  PyObject_HEAD
  ogg_packet *packet;
} PyOggPacketObject;

extern PyTypeObject PyOggPacket_Type;

#define PyOggPacket_AsOggPacket(x) (((PyOggPacketObject *) (x))->packet)

PyObject *PyOggPacket_FromPacket(ogg_packet *);

#endif /* PYOGGPACKET_H */
