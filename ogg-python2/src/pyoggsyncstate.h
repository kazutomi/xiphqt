#ifndef PYOGGSYNCSTATE_H
#define PYOGGSYNCSTATE_H

#include "general.h"

typedef struct {
  PyObject_HEAD
  ogg_sync_state *sync;
} PyOggSyncStateObject;

#define PyOggSyncState_AsOggSyncState(x) (((PyOggSyncStateObject *) (x))->sync)

extern PyTypeObject PyOggSyncState_Type;

PyObject *PyOggSyncState_New(PyObject *, PyObject *);

#endif
