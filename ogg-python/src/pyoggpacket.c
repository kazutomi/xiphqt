#include "pyoggpacket.h"

/************************************************************
			 OggPacket Object
 ************************************************************/

char py_ogg_packet_doc[] = "";

static void py_ogg_packet_dealloc(PyObject *);
static PyObject* py_ogg_packet_getattr(PyObject *, char *);
static int py_ogg_packet_setattr(PyObject *, char *, PyObject *);
static PyObject *py_ogg_packet_repr(PyObject *self);

PyTypeObject py_ogg_packet_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "OggPacket",
  sizeof(py_ogg_packet),
  0,
  
  /* Standard Methods */
  /* (destructor) */ py_ogg_packet_dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ py_ogg_packet_getattr,
  /* (setattrfunc) */ py_ogg_packet_setattr,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ py_ogg_packet_repr,
  
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
  py_ogg_packet_doc
};

static PyMethodDef py_ogg_packet_methods[] = {
  {NULL, NULL}
};

PyObject *
py_ogg_packet_from_packet(ogg_packet *op)
{
  py_ogg_packet *ret = 
    (py_ogg_packet *) PyObject_NEW(py_ogg_packet, 
				   &py_ogg_packet_type);
  if (ret == NULL)
    return NULL;
  ret->op = *op;
  return (PyObject *)ret;
}

static void
py_ogg_packet_dealloc(PyObject *self)
{
  PyMem_DEL(self);
}

static PyObject*
py_ogg_packet_getattr(PyObject *self, char *name)
{
  if (strcmp(name, "granulepos") == 0)
    return PyLong_FromLongLong(PY_OGG_PACKET(self)->granulepos);
  return Py_FindMethod(py_ogg_packet_methods, self, name);
}

static int
py_ogg_packet_setattr(PyObject *self, char *name, PyObject *value)
{
  if (strcmp(name, "granulepos") == 0) {
    ogg_int64_t v = arg_to_64(value);
    PyObject *err = PyErr_Occurred();
    if (err)
      return -1;
    PY_OGG_PACKET(self)->granulepos = v;
    return 0;
  }

  return -1;
}

static PyObject *
py_ogg_packet_repr(PyObject *self)
{
  ogg_packet *op = PY_OGG_PACKET(self);
  char buf[256];
  char *bos = op->b_o_s ? "BOS " : "";
  char *eos = op->e_o_s ? "EOS " : "";

  sprintf(buf, "<OggPacket, %s%spacketno = %lld, granulepos = %lld,"
	  " length = %ld at %p>", bos, eos, op->packetno,
	  op->granulepos, op->bytes, self);
  return PyString_FromString(buf);
}

