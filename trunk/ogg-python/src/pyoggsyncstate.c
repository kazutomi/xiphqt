#include "pyoggsyncstate.h"
#include "pyoggpage.h"
#include "general.h"
#include "_oggmodule.h"

/*****************************************************************
			    OggSyncState Object
 *****************************************************************/

char py_ogg_sync_state_doc[] = "";

static void py_ogg_sync_state_dealloc(PyObject *);
static PyObject* py_ogg_sync_state_getattr(PyObject *, char *);

FDEF(ogg_sync_clear) "Clear the contents of this object.";
FDEF(ogg_sync_reset) "";
#if 0
FDEF(ogg_sync_wrote) "Tell how many bytes were written to the buffer.";
#endif
FDEF(ogg_sync_bytesin) "Append bytes to the sync state buffer.";
FDEF(ogg_sync_pageseek) "Synchronize with the given OggPage.";

PyTypeObject py_ogg_sync_state_type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggSyncState",
  sizeof(py_ogg_sync_state),
  0,
  
  /* Standard Methods */
  /* (destructor) */ py_ogg_sync_state_dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ py_ogg_sync_state_getattr,
  /* (setattrfunc) */ 0,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ 0,
  
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

/*
   TODO: Remove reset functions? Not useful in Python?
*/
static PyMethodDef py_ogg_sync_state_methods[] = {
  {"reset", py_ogg_sync_reset,
   METH_VARARGS, py_ogg_sync_reset_doc},
  {"clear", py_ogg_sync_clear,
   METH_VARARGS, py_ogg_sync_clear_doc},
#if 0
  {"wrote", py_ogg_sync_wrote,
   METH_VARARGS, py_ogg_sync_wrote_doc},
#endif
  {"bytesin", py_ogg_sync_bytesin,
   METH_VARARGS, py_ogg_sync_bytesin_doc},
  {"pageseek", py_ogg_sync_pageseek,
   METH_VARARGS, py_ogg_sync_pageseek_doc},
  {NULL, NULL}
};

PyObject *
py_ogg_sync_state_new(PyObject *self, PyObject *args)
{
  py_ogg_sync_state *ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  ret = PyObject_NEW(py_ogg_sync_state, &py_ogg_sync_state_type);

  if (ret == NULL) 
    return NULL;

  ogg_sync_init(PY_OGG_SYNC_STATE(ret));
  return (PyObject *) ret;
}

static void 
py_ogg_sync_state_dealloc(PyObject *self)
{
  ogg_sync_clear(PY_OGG_SYNC_STATE(self));
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
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  ret = ogg_sync_reset(PY_OGG_SYNC_STATE(self));
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_ogg_sync_clear(PyObject *self, PyObject *args)
{
  int ret;
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  ret = ogg_sync_clear(PY_OGG_SYNC_STATE(self));
  Py_INCREF(Py_None);
  return Py_None;
}

#if 0
static PyObject *
py_ogg_sync_wrote(PyObject *self, PyObject *args)
{
  long bytes;
  int ret; 
  if (!PyArg_ParseTuple(args, "l", &bytes))
    return NULL;

  ret = ogg_sync_wrote(PY_OGG_SYNC_STATE(self), bytes);
  if (ret == -1) {
    PyErr_SetString(Py_OggError, "Overflow of ogg_sync_state buffer.");
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}
#endif


static PyObject *
py_ogg_sync_bytesin(PyObject *self, PyObject *args)
{
  char *bytes;
  int byte_count;
  char *ogg_buffer;
  int ret;

  if (!PyArg_ParseTuple(args, "s#", &bytes, &byte_count))
    return NULL;

  // TODO: ogg_sync_buffer should be modified to fail gracefully when
  // out of memory
  ogg_buffer = ogg_sync_buffer(PY_OGG_SYNC_STATE(self), byte_count);
  memcpy(ogg_buffer, bytes, byte_count);
  ret = ogg_sync_wrote(PY_OGG_SYNC_STATE(self), byte_count);
  if (ret == -1) {
    PyErr_SetString(Py_OggError, "internal error: wrote too much!");
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_ogg_sync_pageseek(PyObject *self, PyObject *args) 
{
  py_ogg_page page;
  int skipped;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  skipped = ogg_sync_pageseek(PY_OGG_SYNC_STATE(self), &page.op);
  if (skipped > 0)
    return Py_BuildValue("iO", skipped, py_ogg_page_from_page(&page.op));
  else
    return Py_BuildValue("iO", skipped, Py_None);
}
