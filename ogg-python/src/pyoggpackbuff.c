#include "pyoggpackbuff.h"
#include "general.h"
#include "_oggmodule.h"

/************************************************************
			 OggPackBuffer Object
 ************************************************************/

char py_oggpack_buffer_doc[] = "";

static void py_oggpack_buffer_dealloc(py_oggpack_buffer *);
static PyObject* py_oggpack_buffer_getattr(PyObject *, char *);

FDEF(oggpack_reset) "";
FDEF(oggpack_writeclear) "";

FDEF(oggpack_look) "";
FDEF(oggpack_look_huff) "";
FDEF(oggpack_look1) "";

FDEF(oggpack_bytes) "";
FDEF(oggpack_bits) "";

FDEF(oggpack_read) "Return the value of n bits";
FDEF(oggpack_read1) "";

FDEF(oggpack_adv) "Advance the read location by n bits";
FDEF(oggpack_adv_huff) "";
FDEF(oggpack_adv1) "";

FDEF(oggpack_write) "Write bits to the buffer.\n\n\
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
  {"reset", py_oggpack_reset,
   METH_VARARGS, py_oggpack_reset_doc},
  {"writeclear", py_oggpack_writeclear,
   METH_VARARGS, py_oggpack_writeclear_doc},
  {"look", py_oggpack_look,
   METH_VARARGS, py_oggpack_look_doc},
  {"look_huff", py_oggpack_look_huff,
   METH_VARARGS, py_oggpack_look_huff_doc},
  {"look1", py_oggpack_look1,
   METH_VARARGS, py_oggpack_look1_doc},
  {"bytes", py_oggpack_bytes,
   METH_VARARGS, py_oggpack_bytes_doc},
  {"bits", py_oggpack_bits,
   METH_VARARGS, py_oggpack_bits_doc},
  {"read", py_oggpack_read,
   METH_VARARGS, py_oggpack_read_doc},
  {"read1", py_oggpack_read1,
   METH_VARARGS, py_oggpack_read1_doc},
  {"write", py_oggpack_write,
   METH_VARARGS, py_oggpack_write_doc},
  {"adv", py_oggpack_adv,
   METH_VARARGS, py_oggpack_adv_doc},
  {"adv_huff", py_oggpack_adv_huff,
   METH_VARARGS, py_oggpack_adv_huff_doc},
  {"adv1", py_oggpack_adv1,
   METH_VARARGS, py_oggpack_adv1_doc},
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
py_oggpack_reset(PyObject *self, PyObject *args)
{
  oggpack_reset(PY_OGGPACK_BUFF(self));
}

static PyObject *
py_oggpack_writeclear(PyObject *self, PyObject *args)
{
  oggpack_writeclear(PY_OGGPACK_BUFF(self));
}

static PyObject *
py_oggpack_look(PyObject *self, PyObject *args) 
{
  int bits = 32;
  long ret;
  if (!PyArg_ParseTuple(args, "l", &bits))
    return NULL;

  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot look at more than 32 bits");
    return NULL;
  }

  ret = oggpack_look(PY_OGGPACK_BUFF(self), bits);
  return PyLong_FromLong(ret);
}

static PyObject *
py_oggpack_look_huff(PyObject *self, PyObject *args) 
{
  int bits = 8;
  long ret;
  if (!PyArg_ParseTuple(args, "l", &bits))
    return NULL;

  if (bits > 8) {
    PyErr_SetString(PyExc_ValueError, "Cannot look at more than 8 bits");
    return NULL;
  }

  ret = oggpack_look_huff(PY_OGGPACK_BUFF(self), bits);
  return PyLong_FromLong(ret);
}

static PyObject *
py_oggpack_look1(PyObject *self, PyObject *args) 
{
  long ret;
  ret = oggpack_look1(PY_OGGPACK_BUFF(self));
  return PyLong_FromLong(ret);
}

static PyObject *
py_oggpack_bytes(PyObject *self, PyObject *args)
{
  long ret = oggpack_bytes(PY_OGGPACK_BUFF(self));
  return PyLong_FromLong(ret);
}

static PyObject *
py_oggpack_bits(PyObject *self, PyObject *args)
{
  long ret = oggpack_bits(PY_OGGPACK_BUFF(self));
  return PyLong_FromLong(ret);
}

static PyObject *
py_oggpack_write(PyObject *self, PyObject *args) 
{
  long val;
  int bits = 32;

  if (!PyArg_ParseTuple(args, "l|l", &val, &bits)) 
    return NULL;

  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot write more than 32 bits");
    return NULL;
  }

  oggpack_write(PY_OGGPACK_BUFF(self), val, bits);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_oggpack_read(PyObject *self, PyObject *args) 
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
py_oggpack_read1(PyObject *self, PyObject *args)
{
  long ret = oggpack_read1(PY_OGGPACK_BUFF(self));
  return PyInt_FromLong(ret);
}

static PyObject *
py_oggpack_adv(PyObject *self, PyObject *args)
{
  int bits;

  if (!PyArg_ParseTuple(args, "i", &bits))
    return NULL;

  oggpack_adv(PY_OGGPACK_BUFF(self), bits);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_oggpack_adv_huff(PyObject *self, PyObject *args)
{
  int bits;

  if (!PyArg_ParseTuple(args, "i", &bits))
    return NULL;

  oggpack_adv_huff(PY_OGGPACK_BUFF(self), bits);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_oggpack_adv1(PyObject *self, PyObject *args)
{
  oggpack_adv1(PY_OGGPACK_BUFF(self));
  Py_INCREF(Py_None);
  return Py_None;
}
