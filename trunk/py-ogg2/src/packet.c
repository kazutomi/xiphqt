#include "packet.h"
#include "general.h"
#include "module.h"

/************************************************************
			 OggPacket Object
 ************************************************************/

char PyOggPacket_Doc[] = "";

static void PyOggPacket_Dealloc(PyObject *);
static PyObject* PyOggPacket_Getattr(PyObject *, char *);
static int PyOggPacket_Setattr(PyObject *, char *, PyObject *);
static PyObject *PyOggPacket_Repr(PyObject *self);
static int PyOggPacket_Size(PyObject *self);

PySequenceMethods PyOggPacket_SeqMethods = {
  PyOggPacket_Size,     /* (length) */
  0,                    /* (concat) */
  0,                    /* (repeat) */
  0,                    /* (item) */
  0,                    /* (slice) */
  0,                    /* (ass_item) */
  0,                    /* (ass_slice) */
  0,                    /* (contains) */
  0,                    /* (inplace_concat) */
  0,                    /* (inplace_repeat) */
};

PyTypeObject PyOggPacket_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggPacket",
  sizeof(PyOggPacketObject),
  0,
  
  /* Standard Methods */
  PyOggPacket_Dealloc,		/* (destructor) */ 
  0, 				/* (printfunc) */ 
  PyOggPacket_Getattr,		/* (getattrfunc) */ 
  PyOggPacket_Setattr,		/* (setattrfunc) */
  0,				/* (cmpfunc) */
  PyOggPacket_Repr,		/* (reprfunc) */
  
  /* Type Categories */
  0, 				/* as number */
  &PyOggPacket_SeqMethods, 	/* as sequence */
  0, 				/* as mapping */
  0, 				/* hash */
  0, 				/* binary */
  0, 				/* repr */
  0, 				/* getattro */
  0, 				/* setattro */
  0, 				/* as buffer */
  0, 				/* tp_flags */
  PyOggPacket_Doc
};

static PyMethodDef PyOggPacket_methods[] = {
  {"bos", NULL},
  {"eos", NULL},
  {"top_granule", NULL},
  {"end_granule", NULL},
  {"packetno", NULL},
  {NULL, NULL}
};

PyOggPacketObject * 
PyOggPacket_Alloc() 
{
  ogg2_packet *packet;
  PyOggPacketObject *ret;

  ret = (PyOggPacketObject *) PyObject_NEW(PyOggPacketObject,
                                           &PyOggPacket_Type);
  if (ret == NULL)
    return NULL;

  ret->valid_flag = 1;

  ret->packet = PyMem_New(ogg2_packet, 1);
  if (ret->packet == NULL) {
    PyObject_Del(ret);
    return NULL;
  }
  memset(ret->packet, 0, sizeof(ogg2_packet));
  return ret;
}


static void
PyOggPacket_Dealloc(PyObject *self)
{
  ogg2_packet_release(PyOggPacket_AsOggPacket(self));
  PyMem_Del(PyOggPacket_AsOggPacket(self));
  PyObject_DEL(self);
}

static int
PyOggPacket_Size(PyObject *self) {

  if (((PyOggPacketObject *) self)->valid_flag == 0) {
    PyErr_SetString(PyOggPacket_Error, "this packet is no longer usable.");
    return -1;
  }

  return PyOggPacket_AsOggPacket(self)->bytes;
}

static PyObject*
PyOggPacket_Getattr(PyObject *self, char *name)
{
  if (((PyOggPacketObject *) self)->valid_flag == 0) {
    PyErr_SetString(PyOggPacket_Error, "this packet is no longer usable.");
    return NULL;
  }

  if (strcmp(name, "bos") == 0)
    return PyLong_FromLong(PyOggPacket_AsOggPacket(self)->b_o_s);
  if (strcmp(name, "eos") == 0)
    return PyLong_FromLong(PyOggPacket_AsOggPacket(self)->e_o_s);
  if (strcmp(name, "top_granule") == 0)
    return PyLong_FromLongLong(PyOggPacket_AsOggPacket(self)->top_granule);
  if (strcmp(name, "end_granule") == 0)
    return PyLong_FromLongLong(PyOggPacket_AsOggPacket(self)->end_granule);
  if (strcmp(name, "packetno") == 0)
    return PyLong_FromLongLong(PyOggPacket_AsOggPacket(self)->packetno);
  return Py_FindMethod(PyOggPacket_methods, self, name);
}

static int
PyOggPacket_Setattr(PyObject *self, char *name, PyObject *value)
{
  if (((PyOggPacketObject *) self)->valid_flag == 0) {
    PyErr_SetString(PyOggPacket_Error, "this packet is no longer usable.");
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
  if (strcmp(name, "top_granule") == 0) {
    ogg_int64_t v;
    if (!arg_to_int64(value, &v))
      return -1;
    PyOggPacket_AsOggPacket(self)->top_granule = v;
    return 0;
  }
  if (strcmp(name, "end_granule") == 0) {
    ogg_int64_t v;
    if (!arg_to_int64(value, &v))
      return -1;
    PyOggPacket_AsOggPacket(self)->end_granule = v;
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
  char *bos;
  char *eos;

  if (((PyOggPacketObject *) self)->valid_flag == 0) {
    sprintf(buf, "<OggPacket that has been passed back to libogg2>");
    return PyString_FromString(buf);
  }

  bos = PyOggPacket_AsOggPacket(self)->b_o_s ? "BOS, " : "";
  eos = PyOggPacket_AsOggPacket(self)->e_o_s ? "EOS, " : "";
  sprintf(buf, "<OggPacket, %s%spacketno = %lld, topgranule = %lld,"
          " endgranule = %lld, length = %ld at %p (%p)>", bos, eos, 
          PyOggPacket_AsOggPacket(self)->packetno,
	  PyOggPacket_AsOggPacket(self)->top_granule, 
	  PyOggPacket_AsOggPacket(self)->end_granule, 
          PyOggPacket_AsOggPacket(self)->bytes, self,
          PyOggPacket_AsOggPacket(self)->packet);
  return PyString_FromString(buf);
}
