#include <vorbis/codec.h>
#include <pyogg/pyogg.h>

#include "general.h"
#include "_oggmodule.h"

#include "pyoggstreamstate.h"
#include "pyoggpacket.h"

static PyMethodDef Ogg_methods[] = {
  {"OggStreamState", py_ogg_stream_state_new, 
   METH_VARARGS, py_ogg_stream_state_doc},
  {NULL, NULL}
};

static char docstring[] = "";

static ogg_module_info mi = {
  VERSION_MINOR,
  VERSION_MAJOR,
  &py_ogg_packet_type,
  &py_ogg_stream_state_type,
  NULL,                          /* Will be Py_OggError */
  py_ogg_packet_from_packet,
};

void
init_ogg(void)
{
  PyObject *module, *dict, *Py_module_info;
  
  module = Py_InitModule("_ogg", Ogg_methods);
  dict = PyModule_GetDict(module);

  Py_OggError = PyErr_NewException("ogg.OggError", NULL, NULL);
  PyDict_SetItemString(dict, "OggError", Py_OggError);

  mi.Py_OggError = Py_OggError;

  Py_module_info = PyCObject_FromVoidPtr(&mi, NULL);
  PyDict_SetItemString(dict, "_moduleinfo", Py_module_info);

  PyDict_SetItemString(dict, "__doc__", PyString_FromString(docstring));
  PyDict_SetItemString(dict, "__version__", PyString_FromString(VERSION));

  if (PyErr_Occurred())
    PyErr_SetString(PyExc_ImportError, "_ogg: init failed");
}




