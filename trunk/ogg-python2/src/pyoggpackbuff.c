#include "pyoggpackbuff.h"
#include "pyoggpacket.h"
#include "general.h"
#include "_ogg2module.h"

/************************************************************
			 OggPackBuffer Object
************************************************************/

char PyOggPackBuffer_Doc[] = "";

static void PyOggPackBuffer_Dealloc(PyObject *);
static PyObject* PyOggPackBuffer_Getattr(PyObject *, char *);
static PyObject *PyOggPackBuffer_Repr(PyObject *self);

FDEF(PyOggPackBuffer_Reset) "Clears and resets the packet buffer";

FDEF(PyOggPackBuffer_Look) "Return the value of n bits without advancing pointer";
FDEF(PyOggPackBuffer_Look1) "Return the value of 1 bit without advancing pointer";

FDEF(PyOggPackBuffer_Bytes) "Return the number of bytes in the buffer";
FDEF(PyOggPackBuffer_Bits) "Return the number of bits in the buffer";

FDEF(PyOggPackBuffer_Read) "Return the value of n bits";
FDEF(PyOggPackBuffer_Read1) "Return the value of 1 bit";

FDEF(PyOggPackBuffer_Adv) "Advance the read location by n bits";
FDEF(PyOggPackBuffer_Adv1) "Advance the read location by 1 bit";

FDEF(PyOggPackBuffer_Export) "Export the OggPacket built by the buffer";

FDEF(PyOggPackBuffer_Write) "Write bits to the buffer.\n\n\
The first parameter is an integer from which the bits will be extracted.\n\
The second parameter is the number of bits to write (defaults to 32)";

PyTypeObject PyOggPackBuffer_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "Oggpack_Buffer",
  sizeof(PyOggPackBufferObject),
  0,
  
  /* Standard Methods */
  /* (destructor) */ PyOggPackBuffer_Dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ PyOggPackBuffer_Getattr,
  /* (setattrfunc) */ 0,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ PyOggPackBuffer_Repr,
  
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
  PyOggPackBuffer_Doc
};

static PyMethodDef PyOggPackBuffer_methods[] = {
  {"reset", PyOggPackBuffer_Reset,
   METH_VARARGS, PyOggPackBuffer_Reset_Doc},
  {"look", PyOggPackBuffer_Look,
   METH_VARARGS, PyOggPackBuffer_Look_Doc},
  {"look1", PyOggPackBuffer_Look1,
   METH_VARARGS, PyOggPackBuffer_Look1_Doc},
  {"bytes", PyOggPackBuffer_Bytes,
   METH_VARARGS, PyOggPackBuffer_Bytes_Doc},
  {"bits", PyOggPackBuffer_Bits,
   METH_VARARGS, PyOggPackBuffer_Bits_Doc},
  {"read", PyOggPackBuffer_Read,
   METH_VARARGS, PyOggPackBuffer_Read_Doc},
  {"read1", PyOggPackBuffer_Read1,
   METH_VARARGS, PyOggPackBuffer_Read1_Doc},
  {"write", PyOggPackBuffer_Write,
   METH_VARARGS, PyOggPackBuffer_Write_Doc},
  {"adv", PyOggPackBuffer_Adv,
   METH_VARARGS, PyOggPackBuffer_Adv_Doc},
  {"adv1", PyOggPackBuffer_Adv1,
   METH_VARARGS, PyOggPackBuffer_Adv1_Doc},
  {"export", PyOggPackBuffer_Export,
   METH_VARARGS, PyOggPackBuffer_Export_Doc},
  {NULL, NULL}  
};

static void
PyOggPackBuffer_Dealloc(PyObject *self)
{
  oggpack_writeclear(PyOggPackBuffer_AsOggPackBuffer(self));
  PyObject_DEL(self);
}

static PyObject*
PyOggPackBuffer_Getattr(PyObject *self, char *name)
{
  return Py_FindMethod(PyOggPackBuffer_methods, self, name);
}

PyObject *
PyOggPackBuffer_New(PyObject *self, PyObject *args) 
{
  PyOggPackBufferObject *ret;
  oggpack_buffer *buffer;
  ogg_buffer_state *buffstate;

  if (!PyArg_ParseTuple(args, "")) 
    return NULL;

  ret = (PyOggPackBufferObject *) PyObject_NEW(PyOggPackBufferObject,
                                               &PyOggPackBuffer_type);
  if (ret == NULL)
    return NULL;

  oggpack_writeinit(buffer, buffstate);
  return (PyObject *)ret;
  }

  if (PyArg_ParseTuple(args, "O!", &PyOggPacket_Type,
                       (PyObject *) &packetobj)) {
    oggpack_readinit(&ret->ob, packetobj->op.packet,
                     packetobj->op.bytes);
    return (PyObject *)ret;
  }

    return NULL;
}

static PyObject *
PyOggPackBuffer_Reset(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  oggpack_reset(PyOggPackBuffer_AsOggPackBuffer(self));
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyOggPackBuffer_Look(PyObject *self, PyObject *args) 
{
  int bits = 32;
  long ret;
  if (!PyArg_ParseTuple(args, "l", &bits))
    return NULL;

  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot look at more than 32 bits");
    return NULL;
  }

  ret = oggpack_look(PyOggPackBuffer_AsOggPackBuffer(self), bits);
  return PyLong_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Look1(PyObject *self, PyObject *args) 
{
  long ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  ret = oggpack_look1(PyOggPackBuffer_AsOggPackBuffer(self));
  return PyLong_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Bytes(PyObject *self, PyObject *args)
{
  long ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  ret = oggpack_bytes(PyOggPackBuffer_AsOggPackBuffer(self));
  return PyLong_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Bits(PyObject *self, PyObject *args)
{
  long ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  ret = oggpack_bits(PyOggPackBuffer_AsOggPackBuffer(self));
  return PyLong_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Write(PyObject *self, PyObject *args) 
{
  long val;
  int bits = 32;

  if (!PyArg_ParseTuple(args, "l|l", &val, &bits)) 
    return NULL;

  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot write more than 32 bits");
    return NULL;
  }

  oggpack_write(PyOggPackBuffer_AsOggPackBuffer(self), val, bits);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyOggPackBuffer_Read(PyObject *self, PyObject *args) 
{
  int bits = 32;
  long ret;
  
  if (!PyArg_ParseTuple(args, "|i", &bits))
    return NULL;
  
  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot read more than 32 bits");
    return NULL;
  }

  ret = oggpack_read(PyOggPackBuffer_AsOggPackBuffer(self),  bits);

  return PyInt_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Read1(PyObject *self, PyObject *args)
{
  long ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  ret = oggpack_read1(PyOggPackBuffer_AsOggPackBuffer(self));
  return PyInt_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Adv(PyObject *self, PyObject *args)
{
  int bits;

  if (!PyArg_ParseTuple(args, "i", &bits))
    return NULL;

  oggpack_adv(PyOggPackBuffer_AsOggPackBuffer(self), bits);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyOggPackBuffer_Adv1(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  oggpack_adv1(PyOggPackBuffer_AsOggPackBuffer(self));
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
PyOggPackBuffer_Export(PyObject *self, PyObject *args)
{
  ogg_packet op;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  op.packet = oggpack_get_buffer(PyOggPackBuffer_AsOggPackBuffer(self));
  op.bytes = oggpack_bytes(PyOggPackBuffer_AsOggPackBuffer(self));
  op.b_o_s = 0;
  op.e_o_s = 0;
  op.granulepos = 0;
  op.packetno = 0;

  return py_ogg_packet_from_packet(&op);
}
  

static PyObject *
PyOggPackBuffer_Repr(PyObject *self)
{
  oggpack_buffer *ob = PyOggPackBuffer_AsOggPackBuffer(self);
  char buf[256];

  sprintf(buf, "<OggPackBuff, endbyte = %ld, endbit = %d at %p>", ob->endbyte,
					ob->endbit, self);
  return PyString_FromString(buf);
}

