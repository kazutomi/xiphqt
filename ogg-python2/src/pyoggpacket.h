#ifndef PYOGGPACKET_H
#define PYOGGPACKET_H

typedef struct {
  PyObject_HEAD
  ogg_packet *packet;
} PyOggPacketObject;

extern PyTypeObject PyOggPacket_Type;

#define PyOggPacket_AsOggPacket(x) ( ((PyOggPacketObject *) (x))->packet )

PyOggPacketObject *PyOggPacket_Alloc(void);

#endif /* PYOGGPACKET_H */
