#include "pyoggpacket.h"

/************************************************************
			 OggPacket Object
 ************************************************************/

char py_ogg_packet_doc[] = "";

static void py_ogg_packet_dealloc(py_ogg_packet *);
static PyObject* py_ogg_packet_getattr(PyObject *, char *);

PyTypeObject py_ogg_packet_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "OggPacket",
  sizeof(py_ogg_packet),
  0,
  
  /* Standard Methods */
  (destructor) py_ogg_packet_dealloc,
  (printfunc) 0,
  (getattrfunc) py_ogg_packet_getattr,
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
py_ogg_packet_dealloc(py_ogg_packet *self)
{
  PyMem_DEL(self);
}

static PyObject*
py_ogg_packet_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(py_ogg_packet_methods, self, name);
}

