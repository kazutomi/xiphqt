#ifndef __OGG2MODULE_H__
#define __OGG2MODULE_H__

#include <Python.h>

extern PyObject *PyOgg_Error;
extern PyObject *PyOggPage_Error;
extern PyObject *PyOggPacket_Error;

/* Object docstrings */

extern char PyOggStreamState_Doc[];
extern char PyOggPackBuffer_Doc[];
extern char PyOggSyncState_Doc[];

#endif /* __OGG2MODULE_H__ */
