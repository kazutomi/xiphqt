#ifndef __PYOGG_H__
#define __PYOGG_H__

#include <Python.h>
#include <ogg/ogg.h>

/* The struct to hold everything we pass from the _ogg module
 * to its submodules.
 */

typedef struct {
  int version_major;
  int version_minor;

  PyTypeObject *OggPacket_Type;
  PyTypeObject *OggStreamState_Type;
  PyObject *Py_OggError;
  PyObject *(*ogg_packet_from_packet)(ogg_packet *op);
  int (*arg_to_int64)(PyObject *longobj, ogg_int64_t *val);
} ogg_module_info;

/*
  Function to convert Long Python objects into an ogg_int64 value.
  Returns 0 on failure (a Python error will be set)
*/
int arg_to_int64(PyObject *longobj, ogg_int64_t *val);


#endif // __PYOGG_H__









