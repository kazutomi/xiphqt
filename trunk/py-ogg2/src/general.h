#ifndef __GENERAL_H__
#define __GENERAL_H__

#include <Python.h>
#include <ogg/ogg2.h>

#define MSG_SIZE 256
#define KEY_SIZE 1024

#define PY_UNICODE (PY_VERSION_HEX >= 0x01060000)

#define FDEF(x) static PyObject *x(PyObject *self, PyObject *args); \
static char x##_Doc[] = 

int arg_to_int64(PyObject *longobj, ogg_int64_t *val);
int arg_to_int32(PyObject *intobj, ogg_int32_t *val);

PyObject * Py_TrueFalse(int value);

#endif /* __GENERAL_H__ */
