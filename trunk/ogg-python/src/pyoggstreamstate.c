#include "_oggmodule.h"
#include "pyoggstreamstate.h"
#include "pyoggpacket.h"
#include "pyoggpage.h"
#include "general.h"

/*****************************************************************
			    OggStreamState Object
 *****************************************************************/


char py_ogg_stream_state_doc[] = "";

static void py_ogg_stream_state_dealloc(PyObject *);
static PyObject* py_ogg_stream_state_getattr(PyObject *, char *);
static PyObject *py_ogg_stream_repr(PyObject *self);

FDEF(ogg_stream_packetin) "Add a packet to the stream.";
FDEF(ogg_stream_clear) "Clear the contents of the stream state.";
FDEF(ogg_stream_flush) "Produce an ogg page suitable for writing to output.";
FDEF(ogg_stream_eos) "Return whether the end of the stream is reached.";
FDEF(ogg_stream_pageout) "Extract and return an OggPage.";
FDEF(ogg_stream_reset) "Reset the stream state";
FDEF(ogg_stream_pagein) "Write a page to the stream";
FDEF(ogg_stream_packetout) "Extract a packet from the stream";

PyTypeObject py_ogg_stream_state_type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggStreamState",
  sizeof(py_ogg_stream_state),
  0,
  
  /* Standard Methods */
  /* (destructor) */ py_ogg_stream_state_dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ py_ogg_stream_state_getattr,
  /* (setattrfunc) */ 0,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ py_ogg_stream_repr,
  
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
  {"clear", py_ogg_stream_clear,
   METH_VARARGS, py_ogg_stream_clear_doc},
  {"flush", py_ogg_stream_flush,
   METH_VARARGS, py_ogg_stream_flush_doc},
  {"eos", py_ogg_stream_eos,
   METH_VARARGS, py_ogg_stream_eos_doc},
  {"packetin", py_ogg_stream_packetin,
   METH_VARARGS, py_ogg_stream_packetin_doc},
  {"pageout", py_ogg_stream_pageout,
   METH_VARARGS, py_ogg_stream_pageout_doc},
  {"pagein", py_ogg_stream_pagein,
   METH_VARARGS, py_ogg_stream_pagein_doc},
  {"packetout", py_ogg_stream_packetout,
   METH_VARARGS, py_ogg_stream_packetout_doc},
  {"reset", py_ogg_stream_reset,
   METH_VARARGS, py_ogg_stream_reset_doc},
  {NULL, NULL}
};

static void 
py_ogg_stream_state_dealloc(PyObject *self)
{
  ogg_stream_clear(PY_OGG_STREAM(self));
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
py_ogg_stream_clear(PyObject *self, PyObject *args) 
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  ogg_stream_clear(PY_OGG_STREAM(self));
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_ogg_stream_packetin(PyObject *self, PyObject *args)
{
  py_ogg_packet *packetobj;
  
  if (!PyArg_ParseTuple(args, "O!", &py_ogg_packet_type,
			(PyObject *) &packetobj))
    return NULL;
  
  if (ogg_stream_packetin(PY_OGG_STREAM(self), &packetobj->op)) {
    /* currently this can't happen */
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
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
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
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
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
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  return PyInt_FromLong(eos);
}

static PyObject *
py_ogg_stream_reset(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
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
  py_ogg_page *pageobj;
  
  if (!PyArg_ParseTuple(args, "O!", &py_ogg_page_type,
			(PyObject *) &pageobj))
    return NULL;
  
  if (ogg_stream_pagein(PY_OGG_STREAM(self), &pageobj->op)) {
    PyErr_SetString(Py_OggError, "error in ogg_stream_pagein (bad page?)");
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_ogg_stream_packetout(PyObject *self, PyObject *args)
{
  ogg_packet op;
  int res;
  
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  res = ogg_stream_packetout(PY_OGG_STREAM(self), &op);
  if (res == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  } else if (res == -1) {
    PyErr_SetString(Py_OggError, "lost sync");
    return NULL;
  }
  return py_ogg_packet_from_packet(&op);
}

static PyObject *
py_ogg_stream_repr(PyObject *self)
{
  ogg_stream_state *os = PY_OGG_STREAM(self);
  char buf[256];
  char *bos = os->b_o_s ? "BOS " : "";
  char *eos = os->e_o_s ? "EOS " : "";

  sprintf(buf, "<OggStreamState, %s%spageno = %ld, packetno = %lld,"
	  " granulepos = %lld, serialno = %d, at %p>",
	  bos, eos, os->pageno, os->packetno, os->granulepos,
	  os->serialno, self); 
  return PyString_FromString(buf);
}
