#include "aomodule.h"

static ao_option_t *
dict_to_options(PyObject *dict)
{
  int pos = 0;
  PyObject *key, *val;
  ao_option_t *head = NULL;
  int ret;
  

  if (!PyDict_Check(dict)) {
    PyErr_SetString(PyExc_TypeError, "Must be a dictionary");
    return NULL;
  }
  
  while (PyDict_Next(dict, &pos, &key, &val) > 0) {
    if (!PyString_Check(key) || !PyString_Check(val)) {
      PyErr_SetString(PyExc_TypeError, "Option keys may only be strings");
      goto error;
    }

    ret = ao_append_option(&head, PyString_AsString(key), PyString_AsString(val));
    if (ret == 0) {
      PyErr_SetString(Py_aoError, "Error appending options");
      goto error;
    }
  }
  return head;

 error:
  ao_free_options(head);
  return NULL;
}

static char *new_kwlist[] = {"driver_id", "bits", "rate", 
			     "channels", "options"};

static PyObject*
py_ao_new(PyObject *self, PyObject *args, PyObject *kwargs)
{
  uint_32 driver_id, bits, rate, channels;
  char * driver_name;
  PyObject *py_options = NULL;
  ao_option_t *c_options;
  ao_device_t *dev;
  ao_Object *retobj;

  bits = 16;
  rate = 44100;
  channels = 2;
  c_options = NULL;

  if(PyArg_ParseTupleAndKeywords(args, kwargs, "s|lllO!", new_kwlist,
				 &driver_name, &bits, &rate, &channels, 
				 &PyDict_Type, &py_options)) {
    driver_id = ao_get_driver_id(driver_name);
  } else {

    PyErr_Clear();
    if(!(PyArg_ParseTupleAndKeywords(args, kwargs, "i|lllO!", new_kwlist,
				     &driver_id, &bits, &rate, &channels, 
				     &PyDict_Type, &py_options)))
      return NULL;

  }  
  if (py_options && PyDict_Size(py_options) > 0) {
    /* dict_to_options returns NULL on error, so you can't pass
       an empty dictionary. We can skip this then anyway. */

    c_options = dict_to_options(py_options);
    if (!c_options) {
      return NULL;
    }
  }

  if (py_options) {
    Py_DECREF(py_options);
  }

  dev = ao_open(driver_id, bits, rate, channels, c_options);
  ao_free_options(c_options);

  if (dev == NULL) {
    PyErr_SetString(Py_aoError, "Error opening device.");
    return NULL;
  }

  retobj = (ao_Object *) PyObject_NEW(ao_Object, &ao_Type);
  retobj->dev = dev;
  return (PyObject *) retobj;
}

static void
py_ao_dealloc(ao_Object *self)
{
  ao_close(self->dev);
  PyMem_DEL(self);
}

static PyObject *
py_ao_get_driver_id(PyObject *self, PyObject *args)
{
  int driver_id;
  char *str = NULL;

  if (!PyArg_ParseTuple(args, "|s", &str))
    return NULL;

  driver_id = ao_get_driver_id(str); /* akes NULL for default */

  if (driver_id == -1) {
    PyErr_SetString(Py_aoError, "No such driver");
    return NULL;
  }

  return PyInt_FromLong(driver_id);
}

static PyObject *
py_ao_get_driver_info(PyObject *self, PyObject *args)
{
  int driver_id = 0;
  char *driver_name;
  ao_info_t *info; 
  PyObject *retdict;

  if (self != NULL) {

    /* It's a method */
    ao_Object *ao_self = (ao_Object *) self;
    info = ao_self->dev->funcs->get_driver_info();

  } else {

    /* Maybe it's a string */
    if ((PyArg_ParseTuple(args, "s", &driver_name))) {

      driver_id = ao_get_driver_id(driver_name);
      if (driver_id == -1) {
	PyErr_SetString(Py_aoError, "Invalid driver name");
      }

    } else {
      
      /* Maybe it's an int */
      PyErr_Clear();
      if (!(PyArg_ParseTuple(args, "i", &driver_id)))
	return NULL;
    }
    
    info = ao_get_driver_info(driver_id);

  }
  if (!info) {
    PyErr_SetString(Py_aoError, "Error getting info");
    return NULL;
  }

  retdict = PyDict_New();

  PyDict_SetItemString(retdict, "name", 
		       PyString_FromString(info->name));
  PyDict_SetItemString(retdict, "short_name", 
		       PyString_FromString(info->short_name));
  PyDict_SetItemString(retdict, "author", 
		       PyString_FromString(info->author));
  PyDict_SetItemString(retdict, "comment", 
		       PyString_FromString(info->comment));
  return retdict;
}

static PyObject *
py_ao_play(PyObject *self, PyObject *args)
{
  char *output_samples;
  uint_32 num_bytes = 0;
  int len;
  ao_Object *ao_self = (ao_Object *) self;

  if (!(PyArg_ParseTuple(args, "s#|l", &output_samples, &len, &num_bytes)))
    return NULL;
  if (num_bytes == 0)
    num_bytes = len;

  Py_BEGIN_ALLOW_THREADS
  ao_play(ao_self->dev, output_samples, num_bytes);
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject*
py_ao_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(ao_Object_methods, self, name);
}

static PyObject*
py_ao_is_big_endian(PyObject *self, PyObject *args)
{
  return PyInt_FromLong(ao_is_big_endian());
}

#define AddInt(x) PyDict_SetItemString(dict, #x, PyInt_FromLong(x))

void
initao(void)
{
  PyObject *dict, *module, *str;

  module = Py_InitModule("ao", ao_methods);
  dict = PyModule_GetDict(module);

  Py_aoError = PyErr_NewException("ao.aoError", NULL, NULL);
  PyDict_SetItemString(dict, "aoError", Py_aoError);
  Py_DECREF(Py_aoError);

  str = PyString_FromString(docstring);
  PyDict_SetItemString(dict, "__doc__", str);
  Py_DECREF(str);

#ifdef AO_NULL
  AddInt(AO_NULL);
#endif

#ifdef AO_OSS 
  AddInt(AO_OSS); 
#endif

#ifdef AO_IRIX
  AddInt(AO_IRIX);
#endif

#ifdef AO_SOLARIS
  AddInt(AO_SOLARIS);
#endif

#ifdef AO_WIN32
  AddInt(AO_WIN32);
#endif

#ifdef AO_BEOS
  AddInt(AO_BEOS);
#endif

#ifdef AO_ESD 
  AddInt(AO_ESD); 
#endif

#ifdef AO_ALSA
  AddInt(AO_ALSA);
#endif

#ifdef AO_WAV 
  AddInt(AO_WAV);  
#endif

#ifdef AO_RAW 
  AddInt(AO_RAW); 
#endif

#ifdef AO_DRIVERS
  AddInt(AO_DRIVERS);
#endif

  ao_initialize();

  if (PyErr_Occurred()) {
    PyErr_SetString(PyExc_ImportError, "ao: Could not import");
  }
}







