#include "general.h"
#include <ogg/ogg.h>

/*
 * general.c
 *
 * functions which are needed by both the ogg and vorbis modules
 */

// Simple function to turn an object into an ogg_int64_t. Returns 0
// on an error, so you need to use PyErr_Occurred() after calling it.
// It DECREFS the argument in the function.

ogg_int64_t
arg_to_64(PyObject *longobj)
{
  ogg_int64_t val = 0;

  if(PyLong_Check(longobj))
    val = PyLong_AsLongLong(longobj);
  else if (PyInt_Check(longobj))
    val = PyInt_AsLong(longobj);
  else {
    Py_DECREF(longobj);
    PyErr_SetString(PyExc_TypeError, "Argument must be int or long");
  }
  Py_DECREF(longobj);
  return val;
}
