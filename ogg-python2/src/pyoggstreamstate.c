#include "_ogg2module.h"
#include "pyoggstreamstate.h"
#include "pyoggpacket.h"
#include "pyoggpage.h"
#include "general.h"

/*****************************************************************
			    OggStreamState Object
 *****************************************************************/


char PyOggStreamState_Doc[] = "";

static void PyOggStreamState_Dealloc(PyObject *);
static PyObject* PyOggStreamState_Getattr(PyObject *, char *);
static PyObject *PyOggStreamState_Repr(PyObject *self);

FDEF(PyOggStreamState_Packetin) "Add a packet to the stream.";
FDEF(PyOggStreamState_Pageout) "Extract and return an OggPage.";
FDEF(PyOggStreamState_Flush) "Produce an ogg page suitable for writing to output.";
FDEF(PyOggStreamState_Pagein) "Write a page to the stream";
FDEF(PyOggStreamState_Packetout) "Extract a packet from the stream";
FDEF(PyOggStreamState_Packetpeek) "Extract a packet from the stream";
FDEF(PyOggStreamState_Reset) "Reset the stream state";
FDEF(PyOggStreamState_Eos) "Return whether the end of the stream is reached.";

PyTypeObject PyOggStreamState_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggStreamState",
  sizeof(PyOggStreamStateObject),
  0,
  
  /* Standard Methods */
  /* (destructor) */ PyOggStreamState_Dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ PyOggStreamState_Getattr,
  /* (setattrfunc) */ 0,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ PyOggStreamState_Repr,
  
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
  PyOggStreamState_Doc
};

static PyMethodDef PyOggStreamState_methods[] = {
  {"packetin", PyOggStreamState_Packetin,
   METH_VARARGS, PyOggStreamState_Packetin_Doc},
  {"pageout", PyOggStreamState_Pageout,
   METH_VARARGS, PyOggStreamState_Pageout_Doc},
  {"flush", PyOggStreamState_Flush,
   METH_VARARGS, PyOggStreamState_Flush_Doc},
  {"pagein", PyOggStreamState_Pagein,
   METH_VARARGS, PyOggStreamState_Pagein_Doc},
  {"packetout", PyOggStreamState_Packetout,
   METH_VARARGS, PyOggStreamState_Packetout_Doc},
  {"packetpeek", PyOggStreamState_Packetpeek,
   METH_VARARGS, PyOggStreamState_Packetpeek_Doc},
  {"reset", PyOggStreamState_Reset,
   METH_VARARGS, PyOggStreamState_Reset_Doc},
  {"eos", PyOggStreamState_Eos, 
   METH_VARARGS, PyOggStreamState_Eos_Doc},
  {NULL, NULL}
};

static void 
PyOggStreamState_Dealloc(PyObject *self)
{
  ogg_stream_destroy(PyOggStreamState_AsOggStreamState(self));
  PyObject_DEL(self);
}

static PyObject* 
PyOggStreamState_Getattr(PyObject *self, char *name)
{
  return Py_FindMethod(PyOggStreamState_methods, self, name);
}

PyObject *
PyOggStreamState_FromSerialno(int serialno)
{
  PyOggStreamStateObject *ret = PyObject_NEW(PyOggStreamStateObject,
                                             &PyOggStreamState_Type);
  if (ret == NULL)
    return NULL;

  ret->stream = ogg_stream_create(serialno);
  if (ret->stream == NULL) {
    PyObject_DEL(ret);
    return NULL;
  }     
  return (PyObject *) ret;
}

PyObject *
PyOggStreamState_New(PyObject *self, PyObject *args)
{
  int serialno;
  if (!PyArg_ParseTuple(args, "i", &serialno))
    return NULL;
  return PyOggStreamState_FromSerialno(serialno);
}

static PyObject *
PyOggStreamState_Packetin(PyObject *self, PyObject *args)
{
  int ret;
  PyOggPacketObject *packet;
  
  if (!PyArg_ParseTuple(args, "O!", &PyOggPacket_Type,
			(PyObject *) &packet))
    return NULL;
  
  if ( packet->valid_flag == 0 ) {
    PyErr_SetString(PyOggPacket_Error, "this packet is no longer usable.");
    return NULL;
  }

  ret = ogg_stream_packetin(PyOggStreamState_AsOggStreamState(self), 
                            PyOggPacket_AsOggPacket(packet));
  if (ret == OGG_SUCCESS) {
    Py_INCREF(Py_None);
    packet->valid_flag = 0;    
    return Py_None;
  } 
  if (ret == OGG_EEOS) {
    PyErr_SetString(PyOgg_Error, "EOS has been set on this stream");
    return NULL;
  }
  PyErr_SetString(PyOgg_Error, "error in ogg_stream_packetin");
  return NULL;
}


static PyObject *
PyOggStreamState_Pageout(PyObject *self, PyObject *args)
{
  int ret;
  PyOggPageObject *pageobj;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pageobj = PyOggPage_Alloc();
  if ( !pageobj ) {
    PyErr_SetString(PyOgg_Error, "Out of Memory.");
    return NULL;
  }
  ret = ogg_stream_pageout(PyOggStreamState_AsOggStreamState(self),
                           PyOggPage_AsOggPage(pageobj));
  if ( ret == 1 ) return (PyObject *) pageobj;
  Py_DECREF(pageobj);
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggStreamState_Flush(PyObject *self, PyObject *args)
{
  int ret;
  PyOggPageObject *pageobj;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  pageobj = PyOggPage_Alloc();
  if ( !pageobj ) {
    PyErr_SetString(PyOgg_Error, "Out of Memory.");
    return NULL;
  }
  ret = ogg_stream_flush(PyOggStreamState_AsOggStreamState(self), 
                         PyOggPage_AsOggPage(pageobj));
  if ( ret == 1 ) return (PyObject *) pageobj;
  Py_DECREF(pageobj);
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggStreamState_Pagein(PyObject *self, PyObject *args)
{
  int ret;
  PyOggPageObject *page;
  
  if (!PyArg_ParseTuple(args, "O!", &PyOggPage_Type,
			(PyObject *) &page))
    return NULL;

  if ( page->valid_flag == 0 ) {
    PyErr_SetString(PyOggPage_Error, "this page is no longer usable.");
    return NULL;
  }

  ret = ogg_stream_pagein(PyOggStreamState_AsOggStreamState(self), 
                          PyOggPage_AsOggPage(page));
  if (ret == OGG_SUCCESS) {
    Py_INCREF(Py_None);
    page->valid_flag = 0;    
    return Py_None;
  }
  if (ret == OGG_ESERIAL) {
    PyErr_SetString(PyOgg_Error, "Page serial does not match stream.");
    return NULL;
  }
  if (ret == OGG_EVERSION) {
    PyErr_SetString(PyOgg_Error, "Unknown Ogg page version.");
    return NULL;
  } 
  PyErr_SetString(PyOgg_Error, "Unknown return from ogg_stream_pagein.");
  return NULL;
}


static PyObject *
PyOggStreamState_Packetout(PyObject *self, PyObject *args)
{
  int ret;
  PyOggPacketObject *packetobj;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  packetobj = PyOggPacket_Alloc();
  if ( !packetobj ) {
    PyErr_SetString(PyOgg_Error, "Out of Memory.");
    return NULL;
  }

  ret = ogg_stream_packetout(PyOggStreamState_AsOggStreamState(self), 
                             PyOggPacket_AsOggPacket(packetobj));
  if (ret == 1) return (PyObject *) packetobj;
  Py_DECREF(packetobj);

  if (ret == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (ret == OGG_HOLE ) {
    PyErr_SetString(PyOgg_Error, "Hole in data, stream is desynced.");
    return NULL;
  }
  if (ret == OGG_SPAN) {
    PyErr_SetString(PyOgg_Error, "Stream spans ??.");
    return NULL;
  }
  PyErr_SetString(PyOgg_Error, "Unknown return from ogg_stream_packetout.");
  return NULL;
}


static PyObject *
PyOggStreamState_Packetpeek(PyObject *self, PyObject *args)
{
  int ret;
  ogg_packet *packet;

  ret = ogg_stream_packetout(PyOggStreamState_AsOggStreamState(self), packet);
  if (ret == 1) {
    Py_INCREF(Py_True);
    return Py_True;
  }
  if (ret == 0) {
    Py_INCREF(Py_False);
    return Py_False;
  }
  if (ret == OGG_HOLE ) {
    PyErr_SetString(PyOgg_Error, "Hole in data, stream is desynced.");
    return NULL;
  }
  if (ret == OGG_SPAN) {
    PyErr_SetString(PyOgg_Error, "Stream spans ??.");
    return NULL;
  }
  PyErr_SetString(PyOgg_Error, "Unknown return from ogg_stream_packetout.");
  return NULL;
}


static PyObject *
PyOggStreamState_Reset(PyObject *self, PyObject *args)
{
  int ret;
  int serialno = -1;

  if (!PyArg_ParseTuple(args, "|i", &serialno))
    return NULL;

  if ( serialno == -1 ) 
    ret = ogg_stream_reset(PyOggStreamState_AsOggStreamState(self));
  else 
    ret = ogg_stream_reset_serialno(PyOggStreamState_AsOggStreamState(self),serialno);  

  if ( ret == OGG_SUCCESS ) {
    Py_INCREF(Py_None);
    return Py_None;
  } else {
    PyErr_SetString(PyOgg_Error, "Unknown error while resetting stream");
    return NULL;
  }
}


static PyObject*
PyOggStreamState_Eos(PyObject *self, PyObject *args)
{
  int eos;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  eos = ogg_stream_eos(PyOggStreamState_AsOggStreamState(self));
  if ( eos == 0 ) {
    Py_INCREF(Py_False);
    return Py_False;
  } 
  Py_INCREF(Py_True);
  return Py_True;
}


static PyObject *
PyOggStreamState_Repr(PyObject *self)
{
  char buf[256];

  sprintf(buf, "<OggStreamState at %p>", self); 
  return PyString_FromString(buf);
}
