#ifndef PYOGG2PAGE_H
#define PYOGG2PAGE_H

#include <pyogg/pyogg2.h>

typedef struct {
  PyObject_HEAD
  ogg_page *page;
} PyOggPageObject;

#define PyOggPage_AsOggPage(x) ( ((PyOggPageObject *) (x))->page )

extern PyTypeObject PyOggPage_Type;

PyObject *PyOggPage_FromOggPage(ogg_page *page);

#endif
