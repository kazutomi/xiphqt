#include <ogg2/ogg.h>

#include <pyogg/pyogg2.h>

#include "general.h"
#include "_ogg2module.h"

#include "pyoggstreamstate.h"
#include "pyoggsyncstate.h"
#include "pyoggpacket.h"
#include "pyoggpackbuff.h"
#include "pyoggpage.h"

static PyMethodDef Ogg_methods[] = {
  {"OggPackBuff", PyOggPackBuffer_New,
   METH_VARARGS, PyOggPackBuffer_Doc},
  {"OggPackBuffB", PyOggPackBuffer_NewB,
   METH_VARARGS, PyOggPackBuffer_Doc},
  {"OggStreamState", PyOggStreamState_New, 
   METH_VARARGS, PyOggStreamState_Doc},
  {"OggSyncState", PyOggSyncState_New,
   METH_VARARGS, PyOggSyncState_Doc},
  {NULL, NULL}
};

static char docstring[] = "";

/* This stuff is useful to py-ogg2 submodules */

static ogg2_module_info mi = {
  VERSION_MINOR,
  VERSION_MAJOR,
  &PyOggPacket_Type,
  &PyOggStreamState_Type,
  NULL,                          /* Will be PyOggError */
/*  PyOggPacket_Alloc, */
  arg_to_int64,
};

void
init_ogg2(void)
{
  PyObject *module, *dict, *Py_module_info;

/*  PyOggPackBuffer_Type.ob_type = &PyType_Type; */
  PyOggPacket_Type.ob_type = &PyType_Type;
  PyOggPage_Type.ob_type = &PyType_Type;
  PyOggStreamState_Type.ob_type = &PyType_Type;
  PyOggSyncState_Type.ob_type = &PyType_Type;
  
  module = Py_InitModule("_ogg2", Ogg_methods);
  dict = PyModule_GetDict(module);

  PyOggError = PyErr_NewException("ogg2.OggError", NULL, NULL);
  PyDict_SetItemString(dict, "OggError", PyOggError);

  mi.PyOggError = PyOggError;

  Py_module_info = PyCObject_FromVoidPtr(&mi, NULL);
  PyDict_SetItemString(dict, "_moduleinfo", Py_module_info);

  PyDict_SetItemString(dict, "__doc__", PyString_FromString(docstring));
  PyDict_SetItemString(dict, "__version__", PyString_FromString("2.0"));

  if (PyErr_Occurred())
    PyErr_SetString(PyExc_ImportError, "_ogg2: init failed");
}
