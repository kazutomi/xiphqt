#ifndef __PYOGG_H__
#define __PYOGG_H__

#include <Python.h>
#include <ogg2/ogg.h>

/* The struct to hold everything we pass from the _ogg module
 * to its submodules.
 */

typedef struct {
  int version_major;
  int version_minor;

  PyTypeObject *PyOggPacket_Type;
  PyTypeObject *PyOggStreamState_Type;
  PyObject *PyOggError;
/*  PyOggPacketObject *(*PyOggPacket_Alloc)(); */
  int (*arg_to_int64)(PyObject *longobj, ogg_int64_t *val);
} ogg2_module_info;

/*
  Function to convert Long Python objects into an ogg_int64 value.
  Returns 0 on failure (a Python error will be set)
*/
int arg_to_int64(PyObject *longobj, ogg_int64_t *val);

#endif // __PYOGG_H__
