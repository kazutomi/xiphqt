#include "general.h"

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

/* Based on arg_to_int64, for 32-bit functions */

int
arg_to_int32(PyObject *intobj, ogg_int32_t *val)
{
  if (PyInt_Check(intobj))
    *val = PyInt_AsLong(intobj);
  else {
    PyErr_SetString(PyExc_TypeError, "Argument must be int");
    return 0;
  }
  return 1;
}

PyObject *
Py_TrueFalse(int value)
{
  if (value) {
    Py_INCREF(Py_True);
    return Py_True;
  } else {
    Py_INCREF(Py_False);
    return Py_False;
  }
}
