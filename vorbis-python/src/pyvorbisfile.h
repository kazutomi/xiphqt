#ifndef __PYVORBIS_FILE_H__
#define __PYVORBIS_FILE_H__

#include <Python.h>
#include <vorbis/vorbisfile.h>

typedef struct {
  PyObject_HEAD
  OggVorbis_File *ovf;
} py_vorbisfile;

extern PyTypeObject py_vorbisfile_type;

#endif // __PYVORBIS_FILE_H__




