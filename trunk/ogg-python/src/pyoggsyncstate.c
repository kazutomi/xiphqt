#include "pyoggsyncstate.h"
#include "general.h"
#include "_oggmodule.h"

/*****************************************************************
			    OggSyncState Object
 *****************************************************************/

/* TODO : Actually add methods to this object and a way to create one! */

char py_ogg_sync_state_doc[] = "";

static void py_ogg_sync_state_dealloc(py_ogg_sync_state *);
static PyObject* py_ogg_sync_state_getattr(PyObject *, char *);

FDEF(ogg_sync_clear) "";
FDEF(ogg_sync_reset) "";
FDEF(ogg_sync_wrote) "";
FDEF(ogg_stream_pagein) "";
FDEF(ogg_stream_packetout) "";

PyTypeObject py_ogg_sync_state_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "OggSyncState",
  sizeof(py_ogg_sync_state),
  0,
  
  /* Standard Methods */
  (destructor) py_ogg_sync_state_dealloc,
  (printfunc) 0,
  (getattrfunc) py_ogg_sync_state_getattr,
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
  py_ogg_sync_state_doc
};

static PyMethodDef py_ogg_sync_state_methods[] = {
  {"reset", py_ogg_sync_reset,
   METH_VARARGS, py_ogg_sync_reset_doc},
  {"wrote", py_ogg_sync_wrote,
   METH_VARARGS, py_ogg_sync_wrote_doc},
  {NULL, NULL}
};

static void 
py_ogg_sync_state_dealloc(py_ogg_sync_state *self)
{
  PyMem_DEL(self);
}

static PyObject* 
py_ogg_sync_state_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(py_ogg_sync_state_methods, self, name);
}

static PyObject *
py_ogg_sync_reset(PyObject *self, PyObject *args)
{
  int ret;
  ret = ogg_sync_reset(PY_OGG_SYNC_STATE(self));
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_ogg_sync_wrote(PyObject *self, PyObject *args)
{
  long bytes;
  int ret; 
  if (!PyArg_ParseTuple(args, "l", &bytes))
    return NULL;

  ret = ogg_sync_wrote(PY_OGG_SYNC_STATE(self), bytes);
  Py_INCREF(Py_None);
  return Py_None;
}
