#ifndef PYOGG2PAGE_H
#define PYOGG2PAGE_H

#include "general.h"

typedef struct {
  PyObject_HEAD
  int valid_flag;
  ogg_page *page;
} PyOggPageObject;

#define PyOggPage_AsOggPage(x) ( ((PyOggPageObject *) (x))->page )

extern PyTypeObject PyOggPage_Type;

PyOggPageObject *PyOggPage_Alloc(void);

#endif
