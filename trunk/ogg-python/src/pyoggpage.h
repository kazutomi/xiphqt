#ifndef PYOGGPAGE_H
#define PYOGGPAGE_H

#include <ogg/ogg.h>
#include <Python.h>

typedef struct {
  PyObject_HEAD
  ogg_page op;
} py_ogg_page;

#define PY_OGG_PAGE(x) (&(((py_ogg_page *) (x))->op))

extern PyTypeObject py_ogg_page_type;

PyObject *py_ogg_page_from_page(ogg_page *og);

#endif
