#ifndef __GENERAL_H__
#define __GENERAL_H__

#define MSG_SIZE 256
#define KEY_SIZE 1024

#define PY_UNICODE (PY_VERSION_HEX >= 0x01060000)

#define FDEF(x) static PyObject *##x(PyObject *self, PyObject *args); \
static char x##_Doc[] = 

#endif /* __GENERAL_H__ */
