#include "general.h"
#include <pyogg/pyogg.h>
#include <ogg/ogg.h>

/*
 * general.c
 *
 * functions which are needed by both the ogg and vorbis modules
 */

/* Simple function to turn an object into an ogg_int64_t. Returns 0 on
 * an error. Does not DECREF the argument */

int
arg_to_int64(PyObject *longobj, ogg_int64_t *val)
{
  if(PyLong_Check(longobj))
    *val = PyLong_AsLongLong(longobj);
  else if (PyInt_Check(longobj))
    *val = PyInt_AsLong(longobj);
  else {
    PyErr_SetString(PyExc_TypeError, "Argument must be int or long");
    return 0;
  }
  return 1;
}
