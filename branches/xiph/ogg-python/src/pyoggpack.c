#include "pyoggpack.h"
#include "general.h"
#include "_oggmodule.h"

/************************************************************
			 OggPackBuffer Object
 ************************************************************/

/* TODO : Actually add methods to this object and a way to create one! */

char py_oggpack_buffer_doc[] = "";

static void py_oggpack_buffer_dealloc(py_oggpack_buffer *);
static PyObject* py_oggpack_buffer_getattr(PyObject *, char *);

FDEF(oggpack_buffer_read) "Return the value of n bits";

FDEF(oggpack_buffer_adv) "Advance the read location by n bits";

FDEF(oggpack_buffer_write) "Write bits to the buffer.\n\n\
The first parameterf is an integer from which the bits will be extracted.\n\
The second parameter is the number of bits to write (defaults to 32)";

PyTypeObject py_oggpack_buffer_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "Oggpack_Buffer",
  sizeof(py_oggpack_buffer),
  0,
  
  /* Standard Methods */
  (destructor) py_oggpack_buffer_dealloc,
  (printfunc) 0,
  (getattrfunc) py_oggpack_buffer_getattr,
  (setattrfunc) 0,
  (cmpfunc) 0,
  (reprfunc) 0,
  
  /* Type Categories */
  0, /* as number */
  0, /* as sequence */
  0, /* as mapping */
  0, /* hash */
  0, /* binary */
  0, /* repr */
  0, /* getattro */
  0, /* setattro */
  0, /* as buffer */
  0, /* tp_flags */
  py_oggpack_buffer_doc
};

static PyMethodDef py_oggpack_buffer_methods[] = {
  {"read", py_oggpack_buffer_read,
   METH_VARARGS, py_oggpack_buffer_read_doc},
  {"write", py_oggpack_buffer_write,
   METH_VARARGS, py_oggpack_buffer_write_doc},
  {"adv", py_oggpack_buffer_adv,
   METH_VARARGS, py_oggpack_buffer_adv_doc},
  {NULL, NULL}
};

static void
py_oggpack_buffer_dealloc(py_oggpack_buffer *self)
{
  PyMem_DEL(self);
}

static PyObject*
py_oggpack_buffer_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(py_oggpack_buffer_methods, self, name);
}

PyObject *
py_oggpack_buffer_new(PyObject *self, PyObject *args) 
{
  py_oggpack_buffer *ret = 
    (py_oggpack_buffer *) PyObject_NEW(py_oggpack_buffer,
				       &py_oggpack_buffer_type);
  if (ret == NULL)
    return NULL;

  oggpack_writeinit(&ret->ob);
  return (PyObject *)ret;
}

static PyObject *
py_oggpack_buffer_write(PyObject *self, PyObject *args) 
{
  long val;
  int bits = 32;

  if (!PyArg_ParseTuple(args, "l|l", &val, &bits)) 
    return NULL;

  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot read more than 32 bits");
    return NULL;
  }

  oggpack_write(PY_OGGPACK_BUFF(self), val, bits);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_oggpack_buffer_read(PyObject *self, PyObject *args) 
{
  int bits = 32;
  long ret;
  
  if (!PyArg_ParseTuple(args, "|i", &bits))
    return NULL;
  
  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot read more than 32 bits");
    return NULL;
  }

  ret = oggpack_read(PY_OGGPACK_BUFF(self),  bits);

  return PyInt_FromLong(ret);
}

static PyObject *
py_oggpack_buffer_adv(PyObject *self, PyObject *args)
{
  int bits;

  if (!PyArg_ParseTuple(args, "i", &bits))
    return NULL;

  oggpack_adv(PY_OGGPACK_BUFF(self), bits);

  Py_INCREF(Py_None);
  return Py_None;
}
