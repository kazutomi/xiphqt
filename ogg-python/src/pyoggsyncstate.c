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
