#include "sync.h"
#include "page.h"
#include "general.h"
#include "module.h"

/*****************************************************************
			    OggSyncState Object
 *****************************************************************/

char PyOggSyncState_Doc[] = "";

static void PyOggSyncState_Dealloc(PyObject *);
static PyObject* PyOggSyncState_Getattr(PyObject *, char *);

FDEF(PyOggSyncState_Pagein) "Add an Ogg page to the sync state buffer.";
FDEF(PyOggSyncState_Read) "Output bytes as a python buffer.";
FDEF(PyOggSyncState_Output) "Write bytes to a file.";
FDEF(PyOggSyncState_Reset) "Reset the PyOggSyncState object.";
FDEF(PyOggSyncState_Write) "Append bytes from a string.";
FDEF(PyOggSyncState_Input) "Append bytes from a file.";
FDEF(PyOggSyncState_Pageout) "Add an Ogg page to the syn.";

PyTypeObject PyOggSyncState_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggSyncState",
  sizeof(PyOggSyncStateObject),
  0,
  
  /* Standard Methods */
  /* (destructor) */ PyOggSyncState_Dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ PyOggSyncState_Getattr,
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
  PyOggSyncState_Doc
};

static PyMethodDef PyOggSyncState_methods[] = {
  {"pagein", PyOggSyncState_Pagein,
   METH_VARARGS, PyOggSyncState_Pagein_Doc},
  {"read", PyOggSyncState_Read,
   METH_VARARGS, PyOggSyncState_Read_Doc},
  {"output", PyOggSyncState_Output,
   METH_VARARGS, PyOggSyncState_Output_Doc},
  {"reset", PyOggSyncState_Reset,
   METH_VARARGS, PyOggSyncState_Reset_Doc},
  {"write", PyOggSyncState_Write,
   METH_VARARGS, PyOggSyncState_Write_Doc},
  {"input", PyOggSyncState_Input,
   METH_VARARGS, PyOggSyncState_Input_Doc},
  {"pageout", PyOggSyncState_Pageout,
   METH_VARARGS, PyOggSyncState_Pageout_Doc},
  {NULL, NULL}
};

PyObject *
PyOggSyncState_New(PyObject *self, PyObject *args)
{
  PyOggSyncStateObject *ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  ret = PyObject_NEW(PyOggSyncStateObject, &PyOggSyncState_Type);

  if (ret == NULL) 
    return NULL;

  ret->sync = ogg_sync_create();
  return (PyObject *) ret;
}


static void 
PyOggSyncState_Dealloc(PyObject *self)
{
  ogg_sync_destroy(PyOggSyncState_AsOggSyncState(self));
  PyObject_DEL(self);
}


/* This should be changed to return pointer, size, etc */
static PyObject* 
PyOggSyncState_Getattr(PyObject *self, char *name)
{
  return Py_FindMethod(PyOggSyncState_methods, self, name);
}


static PyObject *
PyOggSyncState_Pagein(PyObject *self, PyObject *args)
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
  
  ret = ogg_sync_pagein(PyOggSyncState_AsOggSyncState(self),
                        PyOggPage_AsOggPage(page));

  if (ret == OGG_SUCCESS) {
    Py_INCREF(Py_None);
    page->valid_flag = 0;
    return Py_None;
  }
  PyErr_SetString(PyOgg_Error, "Unknown return from ogg_sync_pagein.");
  return NULL;
}


static PyObject *
PyOggSyncState_Read(PyObject *self, PyObject *args)
{
  int ret;
  int ask_bytes;
  int got_bytes;
  unsigned char *ogg_buffer;
  char *pybuffer;
  PyObject *pybufferobj;
  
  if (!PyArg_ParseTuple(args, "i", &ask_bytes))
    return NULL;

  got_bytes = ogg_sync_bufferout(PyOggSyncState_AsOggSyncState(self),
                                 &ogg_buffer);
  if ( got_bytes < ask_bytes ) ask_bytes = got_bytes;


  pybufferobj = PyBuffer_New(ask_bytes);
  PyObject_AsWriteBuffer(pybufferobj, (void **) &pybuffer, &got_bytes);
  if ( got_bytes < ask_bytes ) ask_bytes = got_bytes;
  if ( got_bytes == 0 ) return pybufferobj; /* If 0, return it now */
  memcpy(pybuffer, ogg_buffer, ask_bytes);
 
  ret = ogg_sync_read(PyOggSyncState_AsOggSyncState(self), ask_bytes);
  if (ret == OGG_SUCCESS) return pybufferobj;

  Py_DECREF(pybufferobj);
  PyErr_SetString(PyOgg_Error, "Unknown return from ogg_sync_read.");
  return NULL;
}
  
static PyObject *
PyOggSyncState_Output(PyObject *self, PyObject *args)
{
  int ret;
  int ask_bytes = -1;
  int got_bytes;
  unsigned char *ogg_buffer;
  FILE *fp;
  PyObject *pyfile;

  if (!PyArg_ParseTuple(args, "O!|i", &PyFile_Type, &pyfile, &ask_bytes))
    return NULL;

  if ( ask_bytes < 0 ) ask_bytes = 4096;
    fp = PyFile_AsFile(pyfile);
  
  got_bytes = ogg_sync_bufferout(PyOggSyncState_AsOggSyncState(self),
                                 &ogg_buffer);

  if ( got_bytes == 0 ) return PyInt_FromLong(0);    
  if ( got_bytes < ask_bytes ) ask_bytes = got_bytes;
  fwrite(ogg_buffer, 1, ask_bytes, fp);

  ret = ogg_sync_read(PyOggSyncState_AsOggSyncState(self), ask_bytes);
  if (ret == OGG_SUCCESS) return PyInt_FromLong(ask_bytes);

  PyErr_SetString(PyOgg_Error, "Unknown return from ogg_sync_read.");
  return NULL;
}


static PyObject *
PyOggSyncState_Reset(PyObject *self, PyObject *args)
{
  int ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  ret = ogg_sync_reset(PyOggSyncState_AsOggSyncState(self));
  if (ret == OGG_SUCCESS) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyErr_SetString(PyOgg_Error, "Unknown error from ogg_sync_reset.");
  return NULL;
}


static PyObject *
PyOggSyncState_Write(PyObject *self, PyObject *args)
{
  int ret;
  char *bytes;
  int byte_count;
  char *ogg_buffer;

  if (!PyArg_ParseTuple(args, "s#", &bytes, &byte_count))
    return NULL;

  ogg_buffer = ogg_sync_bufferin(PyOggSyncState_AsOggSyncState(self), 
                                 byte_count);
  memcpy(ogg_buffer, bytes, byte_count);
  ret = ogg_sync_wrote(PyOggSyncState_AsOggSyncState(self), byte_count);
  if (ret == OGG_SUCCESS) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  PyErr_SetString(PyOgg_Error, "Unknown error from ogg_sync_wrote.");
  return NULL;
}


static PyObject *
PyOggSyncState_Input(PyObject *self, PyObject *args)
{
  int ret;
  int bytes = 4096;
  char *ogg_buffer;
  PyObject *pyfile;
  FILE *fp;

  if (!PyArg_ParseTuple(args, "O!|i", &PyFile_Type, &pyfile, &bytes))
    return NULL;
  
  fp = PyFile_AsFile(pyfile);

  ogg_buffer = ogg_sync_bufferin(PyOggSyncState_AsOggSyncState(self),
                                 bytes);
  bytes = fread(ogg_buffer, 1, bytes, fp);
  ret = ogg_sync_wrote(PyOggSyncState_AsOggSyncState(self),
                       bytes);
  if ( ret == OGG_SUCCESS ) {
    return PyLong_FromLong(bytes);
  }
  PyErr_SetString(PyOgg_Error, "Unknown error from ogg_sync_wrote.");
  return NULL;
}


static PyObject *
PyOggSyncState_Pageout(PyObject *self, PyObject *args) 
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
  ret = ogg_sync_pageout(PyOggSyncState_AsOggSyncState(self),
                         PyOggPage_AsOggPage(pageobj));

  if ( ret == 1 ) return (PyObject *) pageobj;
  Py_DECREF(pageobj);
  if ( ret == 0 ) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  if (ret == OGG_HOLE ) {
    PyErr_SetString(PyOgg_Error, "Hole in data, stream is desynced.");
    return NULL;
  }
  PyErr_SetString(PyOgg_Error, "Unknown error from ogg_sync_pageout.");
  return NULL;
}
