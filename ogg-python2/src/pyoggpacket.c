#include <pyogg/pyogg2.h>
#include "pyoggpacket.h"
#include "general.h"

/************************************************************
			 OggPacket Object
 ************************************************************/

char PyOggPacket_Doc[] = "";

static void PyOggPacket_Dealloc(PyObject *);
static PyObject* PyOggPacket_Getattr(PyObject *, char *);
static int PyOggPacket_Setattr(PyObject *, char *, PyObject *);
static PyObject *PyOggPacket_Repr(PyObject *self);

PyTypeObject PyOggPacket_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggPacket",
  sizeof(PyOggPacketObject),
  0,
  
  /* Standard Methods */
  /* (destructor) */ PyOggPacket_Dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ PyOggPacket_Getattr,
  /* (setattrfunc) */ PyOggPacket_Setattr,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ PyOggPacket_Repr,
  
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
  PyOggPacket_Doc
};

static PyMethodDef PyOggPacket_methods[] = {
  {"data", NULL, NULL},
  {"bos", NULL, NULL},
  {"eos", NULL, NULL},
  {"granulepos", NULL, NULL},
  {"packetno", NULL, NULL},
  {NULL, NULL}
};

PyOggPacketObject * 
PyOggPacket_Alloc() 
{
  ogg_packet *packet;
  PyOggPacketObject *ret;

  ret = (PyOggPacketObject *) PyObject_NEW(PyOggPacketObject,
                                           &PyOggPacket_Type);
  if (ret == NULL)
    return NULL;

  ret->packet = PyMem_New(ogg_packet, 1);
  if (ret->packet == NULL) {
    PyObject_Del(ret);
    return NULL;
  }
  memset(ret->packet, 0, sizeof(ogg_packet));
  return ret;
}


static void
PyOggPacket_Dealloc(PyObject *self)
{
  ogg_packet_release(PyOggPacket_AsOggPacket(self));
  PyMem_DEL(self);
}

static PyObject*
PyOggPacket_Getattr(PyObject *self, char *name)
{
  if (strcmp(name, "data") == 0) 
    return PyString_FromStringAndSize((char *) PyOggPacket_AsOggPacket(self)->packet,
                                      PyOggPacket_AsOggPacket(self)->bytes);
  if (strcmp(name, "bos") == 0)
    return PyLong_FromLong(PyOggPacket_AsOggPacket(self)->b_o_s);
  if (strcmp(name, "eos") == 0)
    return PyLong_FromLong(PyOggPacket_AsOggPacket(self)->e_o_s);
  if (strcmp(name, "granulepos") == 0)
    return PyLong_FromLongLong(PyOggPacket_AsOggPacket(self)->granulepos);
  if (strcmp(name, "packetno") == 0)
    return PyLong_FromLongLong(PyOggPacket_AsOggPacket(self)->packetno);
  return Py_FindMethod(PyOggPacket_methods, self, name);
}

static int
PyOggPacket_Setattr(PyObject *self, char *name, PyObject *value)
{
  if (strcmp(name, "data") == 0) {
    PyErr_SetString(PyExc_AttributeError, "Function not written yet");
    return -1;
  }
  if (strcmp(name, "bos") == 0) {
    ogg_int32_t v;
    if (!arg_to_int32(value, &v))
      return -1;
    PyOggPacket_AsOggPacket(self)->b_o_s = v;
    return 0;
  }
  if (strcmp(name, "eos") == 0) {
    ogg_int32_t v;
    if (!arg_to_int32(value, &v))
      return -1;
    PyOggPacket_AsOggPacket(self)->e_o_s = v;
    return 0;
  }
  if (strcmp(name, "granulepos") == 0) {
    ogg_int64_t v;
    if (!arg_to_int64(value, &v))
      return -1;
    PyOggPacket_AsOggPacket(self)->granulepos = v;
    return 0;
  }
  if (strcmp(name, "packetno") == 0) {
    ogg_int64_t v;
    if (!arg_to_int64(value, &v))
      return -1;
    PyOggPacket_AsOggPacket(self)->packetno = v;
    return 0;
  }
  
  return -1;
}

static PyObject *
PyOggPacket_Repr(PyObject *self)
{
  char buf[256];
  char *bos = PyOggPacket_AsOggPacket(self)->b_o_s ? "BOS, " : "";
  char *eos = PyOggPacket_AsOggPacket(self)->e_o_s ? "EOS, " : "";

  sprintf(buf, "<OggPacket, %s%spacketno = %lld, granulepos = %lld,"
	  " length = %ld at %p>", bos, eos, PyOggPacket_AsOggPacket(self)->packetno,
	  PyOggPacket_AsOggPacket(self)->granulepos, PyOggPacket_AsOggPacket(self)->bytes, self);
  return PyString_FromString(buf);
}
