#include "general.h"
#include "_oggmodule.h"
#include "pyoggpage.h"

/*****************************************************************
			    OggPage Object
 *****************************************************************/


char py_ogg_page_doc[] = "";

static void py_ogg_page_dealloc(py_ogg_page *);
static PyObject* py_ogg_page_getattr(PyObject *, char *);

FDEF(ogg_page_writeout) "Write the page to a given file object. Returns the number of bytes written.";
FDEF(ogg_page_eos) "Tell whether this page is the end of a stream.";
FDEF(ogg_page_version) "Return the stream version";
FDEF(ogg_page_serialno) "Return the serial number of the page";
FDEF(ogg_page_pageno) "Return the page number of the page";

PyTypeObject py_ogg_page_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "OggPage",
  sizeof(py_ogg_page),
  0,
  
  /* Standard Methods */
  (destructor) py_ogg_page_dealloc,
  (printfunc) 0,
  (getattrfunc) py_ogg_page_getattr,
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
  py_ogg_page_doc
};

static PyMethodDef py_ogg_page_methods[] = {
  {"writeout", py_ogg_page_writeout,
   METH_VARARGS, py_ogg_page_writeout_doc},
  {"eos", py_ogg_page_eos,
   METH_VARARGS, py_ogg_page_eos_doc},
  {"version", py_ogg_page_version,
   METH_VARARGS, py_ogg_page_version_doc},
  {"serialno", py_ogg_page_serialno,
   METH_VARARGS, py_ogg_page_serialno_doc},
  {"pageno", py_ogg_page_pageno,
   METH_VARARGS, py_ogg_page_pageno_doc},
  {NULL, NULL}
};

static void 
py_ogg_page_dealloc(py_ogg_page *self)
{
  PyMem_DEL(self);
}

static PyObject* 
py_ogg_page_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(py_ogg_page_methods, self, name);
}

PyObject *
py_ogg_page_from_page(ogg_page *op)
{
  py_ogg_page *pyop = (py_ogg_page *) PyObject_NEW(py_ogg_page,
						   &py_ogg_page_type);
  if (pyop == NULL)
    return NULL;

  pyop->op = *op;
  return (PyObject *) pyop;
}

/* Take in a Python file object and write the given page to that file. */
static PyObject *
py_ogg_page_writeout(PyObject *self, PyObject *args)
{
  FILE *fp;
  int bytes;
  PyObject *pyfile;
  py_ogg_page *op_self = (py_ogg_page *) self;

  if (!PyArg_ParseTuple(args, "O!", &PyFile_Type, &pyfile))
    return NULL;
  
  fp = PyFile_AsFile(pyfile);
  bytes = fwrite(op_self->op.header,1,op_self->op.header_len, fp);
  bytes += fwrite(op_self->op.body,1,op_self->op.body_len, fp);

  return PyInt_FromLong(bytes);

}

static PyObject*
py_ogg_page_eos(PyObject *self, PyObject *args)
{
  int eos = ogg_page_eos(PY_OGG_PAGE(self));
  return PyInt_FromLong(eos);
}

static PyObject *
py_ogg_page_version(PyObject *self, PyObject *args) 
{
  return PyInt_FromLong(ogg_page_version(PY_OGG_PAGE(self)));
}

static PyObject *
py_ogg_page_serialno(PyObject *self, PyObject *args)
{
  return PyInt_FromLong(ogg_page_serialno(PY_OGG_PAGE(self)));
}

static PyObject *
py_ogg_page_pageno(PyObject *self, PyObject *args)
{
  return PyLong_FromLong(ogg_page_pageno(PY_OGG_PAGE(self)));
}
