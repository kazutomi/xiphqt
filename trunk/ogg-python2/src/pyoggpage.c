#include "general.h"
#include "_ogg2module.h"
#include "pyoggpage.h"

/*****************************************************************
			    OggPage Object
 *****************************************************************/


char PyOggPage_Doc[] = "This is an Ogg Page.";

static void PyOggPage_Dealloc(PyObject *);
static PyObject* PyOggPage_Getattr(PyObject *, char *);
static int PyOggPage_Setattr(PyObject *self, char *name, PyObject *value);
static PyObject *PyOggPage_Repr(PyObject *self);
static PyObject *PyOggPage_Size(PyObject *self, PyObject *args);

PyTypeObject PyOggPage_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "OggPage",
  sizeof(PyOggPageObject),
  0,
  
  /* Standard Methods */
  /* (destructor) */ PyOggPage_Dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ PyOggPage_Getattr,
  /* (setattrfunc) */ PyOggPage_Setattr,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ PyOggPage_Repr,
  
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
  PyOggPage_Doc
};

static PyMethodDef PyOggPage_methods[] = {
  {"__len__", PyOggPage_Size, METH_VARARGS, NULL},
/*  {"__getattr__", PyOggPage_Getattr, METH_VARARGS, NULL},
  {"__setattr__", PyOggPage_Setattr, METH_VARARGS, NULL}, */
  {"bos", NULL, NULL, "The BOS (Beginning-Of-Stream) flag."},
  {"body", NULL, NULL, NULL},
  {"continued", NULL, NULL, NULL},
  {"eos", NULL, NULL, NULL},
  {"granulepos", NULL, NULL, NULL},
  {"header", NULL, NULL, NULL},
  {"packets", NULL, NULL, NULL},
  {"pageno", NULL, NULL, NULL},
  {"serialno", NULL, NULL, NULL},
  {"version", NULL, NULL, NULL},
  {NULL, NULL}
};

PyOggPageObject *
PyOggPage_Alloc() {
  ogg_page *page;
  PyOggPageObject *ret;

  ret = (PyOggPageObject *) PyObject_NEW(PyOggPageObject, &PyOggPage_Type);
  if (ret == NULL)
    return NULL;

  page = PyMem_New(ogg_page, 1);
  if (page == NULL) {
    PyObject_Del(ret);
    return NULL;
  }
  memset(page, 0, sizeof(*page));

  ret->page = page;
  return ret;
}

static void 
PyOggPage_Dealloc(PyObject *self) {
  ogg_page_release(PyOggPage_AsOggPage(self));
  PyMem_Del(PyOggPage_AsOggPage(self));
  PyObject_Del(self);
}


static PyObject *
PyOggPage_Size(PyObject *self, PyObject *args) {
  int size;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  size = PyOggPage_AsOggPage(self)->header_len +
         PyOggPage_AsOggPage(self)->body_len;
  return PyInt_FromLong(size);
}

static PyObject* 
PyOggPage_Getattr(PyObject *self, char *name) {
  ogg_page *page = PyOggPage_AsOggPage(self);

  if (strcmp(name, "body") == 0) 
    return PyString_FromStringAndSize((char *) page->body, page->body_len);

  if (strcmp(name, "bos") == 0) {
    if (ogg_page_bos(page)) {
      Py_INCREF(Py_True);
      return Py_True;
    }
    else {
      Py_INCREF(Py_False);
      return Py_False;
    }
  }
  if (strcmp(name, "continued") == 0) {
    if (ogg_page_continued(page)) {
      Py_INCREF(Py_True);
      return Py_True;
    }
    else {
      Py_INCREF(Py_False);
      return Py_False;
    }
  }
  if (strcmp(name, "eos") == 0) {
    if (ogg_page_eos(page)) {
      Py_INCREF(Py_True);
      return Py_True;
    }
    else {
      Py_INCREF(Py_False);
      return Py_False;
    }
  }
  if (strcmp(name, "granulepos") == 0)
    return PyLong_FromLongLong(ogg_page_granulepos(page));
  if (strcmp(name, "packets") == 0) 
    return PyInt_FromLong(ogg_page_packets(page));
  if (strcmp(name, "pageno") == 0) 
    return PyInt_FromLong(ogg_page_pageno(page));
  if (strcmp(name, "serialno") == 0) 
    return PyInt_FromLong(ogg_page_serialno(page));
  if (strcmp(name, "version") == 0) 
    return PyInt_FromLong(ogg_page_version(page));
  return Py_FindMethod(PyOggPage_methods, self, name);
}

static int
PyOggPage_Setattr(PyObject *self, char *name, PyObject *value)
{
  char *head = (char *) PyOggPage_AsOggPage(self)->header;

  if (strcmp(name, "bos") == 0) {
    ogg_int64_t v;

    if (!arg_to_int64(value, &v))
      return -1;
    if (!v) {
      if (head[5] & 0x02) {
        head[5] = head[5] & 0xFD;
        ogg_page_checksum_set(PyOggPage_AsOggPage(self));
      }
    } else {
      if (!(head[5] & 0x02)) {
        head[5] = head[5] | 0x02;
        ogg_page_checksum_set(PyOggPage_AsOggPage(self));
      }
    }
    return 0;
  }

  if (strcmp(name, "continued") == 0) {
    ogg_int64_t v;

    if (!arg_to_int64(value, &v))
      return -1;
    if (!v) {
      if (head[5] & 0x01) {
        head[5] = head[5] & 0xFE;
        ogg_page_checksum_set(PyOggPage_AsOggPage(self));
      }
    } else {
      if (!(head[5] & 0x01)) {
        head[5] = head[5] | 0x01;
        ogg_page_checksum_set(PyOggPage_AsOggPage(self));
      }
    }
    return 0;
  }
 
  if (strcmp(name, "eos") == 0) {
    ogg_int64_t v;

    if (!arg_to_int64(value, &v))
      return -1;
    if (!v) {
      if (head[5] & 0x04) {
        head[5] = head[5] & 0xFB;
        ogg_page_checksum_set(PyOggPage_AsOggPage(self));
      }
    } else {
      if (!(head[5] & 0x04)) {
        head[5] = head[5] | 0x04;
        ogg_page_checksum_set(PyOggPage_AsOggPage(self));
      }
    }
    return 0;
  }

  if (strcmp(name, "granulepos") == 0) {
    int i;
    ogg_int64_t v;
    if (!arg_to_int64(value, &v))
      return -1;
    for (i=6; i<14; i++) {
      head[i] = v & 0xff;
      v >>= 8;
    }
    ogg_page_checksum_set(PyOggPage_AsOggPage(self));
    return 0;
  }

  if (strcmp(name, "pageno") == 0) {
    int i;
    ogg_int32_t v;
    if (!arg_to_int32(value, &v))
      return -1;
    for (i=18; i<22; i++) {
      head[i] = v & 0xff;
      v >>= 8;
    }
    ogg_page_checksum_set(PyOggPage_AsOggPage(self));
    return 0;
  }

  if (strcmp(name, "serialno") == 0) {
    int i;
    ogg_int32_t v;
    if (!arg_to_int32(value, &v))
      return -1;
    for (i=14; i<18; i++) {
      head[i] = v & 0xff;
      v >>= 8;
    }
    ogg_page_checksum_set(PyOggPage_AsOggPage(self));
    return 0;
  }

  if (strcmp(name, "version") == 0) {
    int i;
    ogg_int32_t v;
    if (!arg_to_int32(value, &v))
      return -1;
    if ( v<0 | v>255 ) {
      PyErr_SetString(PyExc_ValueError, "version must be between 0 and 255");  
      return -1;
    }
    head[4] = v;
    ogg_page_checksum_set(PyOggPage_AsOggPage(self));
    return 0;
  }
  PyErr_SetString(PyExc_AttributeError, "OggPage object has no such attribute");  
  return -1;
}

static PyObject *
PyOggPage_Repr(PyObject *self)
{
  char buf[256];
  char *bos = ogg_page_bos(PyOggPage_AsOggPage(self)) ? "BOS, " : "";
  char *eos = ogg_page_eos(PyOggPage_AsOggPage(self)) ? "EOS, " : "";
  char *cont = ogg_page_continued(PyOggPage_AsOggPage(self)) ? "CONT, " : "";
  sprintf(buf, "<OggPage, %s%s%spageno = %ld, granulepos = %lld,"
	  " packets = %d, serialno = %d, version = %d," 
          " head length = %ld, body length = %ld, at %p>",
	  cont, bos, eos, ogg_page_pageno(PyOggPage_AsOggPage(self)),
          ogg_page_granulepos(PyOggPage_AsOggPage(self)),
	  ogg_page_packets(PyOggPage_AsOggPage(self)),
          ogg_page_serialno(PyOggPage_AsOggPage(self)),
          ogg_page_version(PyOggPage_AsOggPage(self)), 
          PyOggPage_AsOggPage(self)->header_len, 
          PyOggPage_AsOggPage(self)->body_len,
          PyOggPage_AsOggPage(self)); 
  return PyString_FromString(buf);
}
