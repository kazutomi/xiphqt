#include "_oggmodule.h"
#include "pyoggstreamstate.h"
#include "pyoggpacket.h"
#include "pyoggpage.h"
#include "general.h"

/*****************************************************************
			    OggStreamState Object
 *****************************************************************/


char py_ogg_stream_state_doc[] = "";

static void py_ogg_stream_state_dealloc(py_ogg_stream_state *);
static PyObject* py_ogg_stream_state_getattr(PyObject *, char *);

FDEF(ogg_stream_packetin) "Add a packet to the stream.";
FDEF(ogg_stream_flush) "Produce an ogg page suitable for writing to output.";
FDEF(ogg_stream_eos) "Not quite sure what it does!!"; /* FIXME */
FDEF(ogg_stream_pageout) "Not quite sure what it does!!"; /* FIXME */
FDEF(ogg_stream_reset) "Reset the stream state";
FDEF(ogg_stream_pagein) "Write a page to the stream";
FDEF(ogg_stream_packetout) "Extract a packet from the stream";

PyTypeObject py_ogg_stream_state_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "OggStreamState",
  sizeof(py_ogg_stream_state),
  0,
  
  /* Standard Methods */
  (destructor) py_ogg_stream_state_dealloc,
  (printfunc) 0,
  (getattrfunc) py_ogg_stream_state_getattr,
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
  py_ogg_stream_state_doc
};

static PyMethodDef py_ogg_stream_state_methods[] = {
  {"packetin", py_ogg_stream_packetin,
   METH_VARARGS, py_ogg_stream_packetin_doc},
  {"flush", py_ogg_stream_flush,
   METH_VARARGS, py_ogg_stream_flush_doc},
  {"eos", py_ogg_stream_eos,
   METH_VARARGS, py_ogg_stream_eos_doc},
  {"pageout", py_ogg_stream_pageout,
   METH_VARARGS, py_ogg_stream_pageout_doc},
  {"reset", py_ogg_stream_reset,
   METH_VARARGS, py_ogg_stream_reset_doc},
  {NULL, NULL}
};

static void 
py_ogg_stream_state_dealloc(py_ogg_stream_state *self)
{
  PyMem_DEL(self);
}

static PyObject* 
py_ogg_stream_state_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(py_ogg_stream_state_methods, self, name);
}

PyObject *
py_ogg_stream_state_from_serialno(int serialno)
{
  py_ogg_stream_state *ret = PyObject_NEW(py_ogg_stream_state,
					  &py_ogg_stream_state_type);

  if (ret == NULL)
    return NULL;

  ogg_stream_init(&ret->os, serialno);
  return (PyObject *) ret;
}

PyObject *
py_ogg_stream_state_new(PyObject *self, PyObject *args)
{
  int serialno;
  if (!PyArg_ParseTuple(args, "i", &serialno))
    return NULL;
  return py_ogg_stream_state_from_serialno(serialno);
}

static PyObject *
py_ogg_stream_packetin(PyObject *self, PyObject *args)
{
  py_ogg_packet *packetobj;
  
  if (!PyArg_ParseTuple(args, "O!", &py_ogg_packet_type,
			(PyObject *) &packetobj))
    return NULL;
  
  if (ogg_stream_packetin(PY_OGG_STREAM(self), &packetobj->op)) {
    PyErr_SetString(Py_OggError, "error in ogg_stream_packetin");
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_ogg_stream_flush(PyObject *self, PyObject *args)
{
  ogg_page op;
  int res;
  
  res = ogg_stream_flush(PY_OGG_STREAM(self), &op);
  if (res == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return py_ogg_page_from_page(&op);
}

/* TODO: You can't keep a page between calls to flush or pageout!! */

static PyObject *
py_ogg_stream_pageout(PyObject *self, PyObject *args)
{
  ogg_page op;
  int res;
  
  res = ogg_stream_pageout(PY_OGG_STREAM(self), &op);
  if (res == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  return py_ogg_page_from_page(&op);
}

static PyObject*
py_ogg_stream_eos(PyObject *self, PyObject *args)
{
  int eos = ogg_stream_eos(PY_OGG_STREAM(self));
  return PyInt_FromLong(eos);
}

static PyObject *
py_ogg_stream_reset(PyObject *self, PyObject *args)
{
  if (ogg_stream_reset(PY_OGG_STREAM(self))) {

    PyErr_SetString(Py_OggError, "Error resetting stream");
    return NULL;

  } else {

    Py_INCREF(Py_None);
    return Py_None;

  }
}

static PyObject *
py_ogg_stream_pagein(PyObject *self, PyObject *args)
{
  int val;
  return PyInt_FromLong(val);
}

static PyObject *
py_ogg_stream_packetout(PyObject *self, PyObject *args)
{
  int val;
  return PyInt_FromLong(val);
}
