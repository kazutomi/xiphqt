#include <assert.h>
#include <vorbis/vorbisenc.h>

#include "general.h"
#include "vorbismodule.h"

#include "pyvorbiscodec.h"
#include "pyvorbisinfo.h"

/**************************************************************
                         VorbisDSP Object
 **************************************************************/

FDEF(vorbis_analysis_headerout) "Create three header packets for an ogg\n\
stream, given an optional comment object.";
FDEF(vorbis_analysis_blockout) "Output a VorbisBlock. Data must be written to the object first.";

FDEF(vorbis_block_init) "Create a VorbisBlock object for use in encoding {more?!}";
FDEF(dsp_write) "Write audio data to the dsp device and have it analyzed. \n\
Each argument must be a string containing the audio data for a\n\
single channel.";
FDEF(dsp_close) "Signal that all audio data has been written to the object.";

static void py_dsp_dealloc(py_dsp *);
static PyObject *py_dsp_getattr(PyObject *, char*);

char py_dsp_doc[] = "";

PyTypeObject py_dsp_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "VorbisDSPState",
  sizeof(py_dsp),
  0,

  /* Standard Methods */
  (destructor) py_dsp_dealloc,
  (printfunc) 0,
  (getattrfunc) py_dsp_getattr,
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
  py_dsp_doc,
};

static PyMethodDef DSP_methods[] = {
  {"headerout", py_vorbis_analysis_headerout,
   METH_VARARGS, py_vorbis_analysis_headerout_doc},
  {"blockout", py_vorbis_analysis_blockout,
   METH_VARARGS, py_vorbis_analysis_blockout_doc},
  {"create_block", py_vorbis_block_init,
   METH_VARARGS, py_vorbis_block_init_doc},
  {"write", py_dsp_write,
   METH_VARARGS, py_dsp_write_doc},
  {"close", py_dsp_close,
   METH_VARARGS, py_dsp_close_doc},
  {NULL, NULL}
};

PyObject *
py_dsp_from_dsp(vorbis_dsp_state *vd, PyObject *parent)
{
  py_dsp *ret = (py_dsp *) PyObject_NEW(py_dsp, &py_dsp_type);

  if (ret == NULL) 
    return NULL;
  
  ret->vd = *vd;
  ret->parent = parent;
  Py_XINCREF(parent);
  return (PyObject *) ret;
}

PyObject *
py_dsp_new(PyObject *self, PyObject *args)
{
  py_vinfo* py_vi;
  py_dsp *ret;
  vorbis_info *vi;
  vorbis_dsp_state vd;
  
  if (!PyArg_ParseTuple(args, "O!", &py_vinfo_type, &py_vi))
    return NULL;
  
  ret = (py_dsp *) PyObject_NEW(py_dsp, &py_dsp_type);
  vi = &py_vi->vi;
  vorbis_synthesis_init(&vd, vi);
  return py_dsp_from_dsp(&vd, (PyObject *) py_vi);
}

static void
py_dsp_dealloc(py_dsp *self)
{
  Py_XDECREF(self->parent);
  PyMem_DEL(self);
}

static PyObject*
py_dsp_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(DSP_methods, self, name);
}

static PyObject *
py_vorbis_analysis_blockout(PyObject *self, PyObject *args)
{
  vorbis_block vb;
  int ret;
  py_dsp *dsp_self = (py_dsp *) self;
  vorbis_block_init(&dsp_self->vd, &vb);
  ret = vorbis_analysis_blockout(&dsp_self->vd, &vb);
  if (ret == 1)
    return py_block_from_block(&vb, self);
  else {
    Py_INCREF(Py_None);
    return Py_None;
  }
}

static PyObject *
py_vorbis_analysis_headerout(PyObject *self, PyObject *args)
{
  int code;
  py_dsp *dsp_self = (py_dsp *) self;
  py_vcomment *comm = NULL;
  vorbis_comment vc;
  ogg_packet header, header_comm, header_code;

  PyObject *pyheader = NULL;
  PyObject *pyheader_comm = NULL;
  PyObject *pyheader_code = NULL;
  PyObject *ret = NULL;
  
  // Takes a comment object as the argument.
  // I'll just give them an empty one if they don't provied one.
  if (!PyArg_ParseTuple(args, "|O!", &py_vcomment_type, &comm))
    return NULL;
  
  if (comm == NULL) {
    vorbis_comment_init(&vc); // Initialize an empty comment struct
  } else {
    vc = comm->vc;
  }
    
  if ((code = vorbis_analysis_headerout(&dsp_self->vd, &vc, &header,
					&header_comm, &header_code))) {
    v_error_from_code(code, "vorbis_analysis_header_out");
    goto finish;
  }
  
  // Returns a tuple of oggpackets (header, header_comm, header_code)
  
  pyheader = modinfo->ogg_packet_from_packet(&header);
  pyheader_comm = modinfo->ogg_packet_from_packet(&header_comm);
  pyheader_code = modinfo->ogg_packet_from_packet(&header_code);
  if (pyheader == NULL || pyheader_comm == NULL || pyheader_code == NULL)
    goto error;
  
  ret = PyTuple_New(3);
  PyTuple_SET_ITEM(ret, 0, pyheader);
  PyTuple_SET_ITEM(ret, 1, pyheader_comm);
  PyTuple_SET_ITEM(ret, 2, pyheader_code);
  
 finish:
  if (comm == NULL) // Get rid of it if we created it
    vorbis_comment_clear(&vc);
  return ret;
 error:
  if (comm == NULL)
    vorbis_comment_clear(&vc);
  Py_XDECREF(pyheader);
  Py_XDECREF(pyheader_comm);
  Py_XDECREF(pyheader_code);
  return NULL;
}

static PyObject *
py_vorbis_block_init(PyObject *self, PyObject *args)
{
  vorbis_block vb;
  py_dsp *dsp_self = (py_dsp *) self;
  PyObject *ret;

  vorbis_block_init(&dsp_self->vd,&vb);
  ret = py_block_from_block(&vb, self);
  return ret;
}

// Returns "len" if all arguments are strings of the same length, 
// -1 if one or more are not strings
// -2 if they have different lengths

#define NON_STRING -1
#define DIFF_LENGTHS -2

static int
string_size(PyObject *args, int size)
{
  PyObject *cur;
  int k;
  int len = -1;

  for (k = 0; k < size; k++) {
    cur = PyTuple_GET_ITEM(args, k);
    if (!PyString_Check(cur))
      return NON_STRING;

    // make sure the lengths are uniform
    if (len == -1)
      len = PyString_Size(cur);
    else
      if (PyString_Size(cur) != len)
	return DIFF_LENGTHS;
  }
  return len;
}

static PyObject *
py_dsp_write(PyObject *self, PyObject *args)
{
  int k, channels;
  char err_msg[256];
  float **buffs;
  float **analysis_buffer;
  int len, samples;

  py_dsp *dsp_self = (py_dsp *) self;
  PyObject *cur;
  
  assert(PyTuple_Check(args));

  channels = dsp_self->vd.vi->channels;

  if (PyTuple_Size(args) != channels) {
    snprintf(err_msg, sizeof(err_msg), 
	     "Expected %d strings as arguments; found %d arguments",
	     channels, PyTuple_Size(args));
    PyErr_SetString(Py_VorbisError, err_msg);
    return NULL;
  }

  len = string_size(args, channels);
  if (len == NON_STRING) {
    PyErr_SetString(Py_VorbisError, 
		    "All arguments must be strings");
    return NULL;
  }
  if (len == DIFF_LENGTHS) {
    PyErr_SetString(Py_VorbisError, 
		    "All arguments must have the same length.");
    return NULL;
  }

  samples = len / sizeof(float);
  
  buffs = (float **) malloc(channels * sizeof(float *));
  for (k = 0; k < channels; k++) {
    cur = PyTuple_GET_ITEM(args, k);
    buffs[k] = (float *) PyString_AsString(cur);
  }

  analysis_buffer = vorbis_analysis_buffer(&dsp_self->vd, len * sizeof(float));
  for (k = 0; k < channels; k++)
    memcpy(analysis_buffer[k], buffs[k], len);

  free(buffs);
  
  vorbis_analysis_wrote(&dsp_self->vd, samples);

  return PyInt_FromLong(samples); // return the number of samples written
}

static PyObject *
py_dsp_close(PyObject *self, PyObject *args)
{
  py_dsp *dsp_self = (py_dsp *) self;
  vorbis_analysis_wrote(&dsp_self->vd, 0);
  Py_INCREF(Py_None);
  return Py_None;
}

/*********************************************************
			VorbisBlock
 *********************************************************/
static void py_block_dealloc(py_block *);
static PyObject *py_block_getattr(PyObject *, char*);

FDEF(vorbis_analysis) "Output an OggPage.";

char py_block_doc[] = "";

PyTypeObject py_block_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "VorbisBlock",
  sizeof(py_block),
  0,

  /* Standard Methods */
  (destructor) py_block_dealloc,
  (printfunc) 0,
  (getattrfunc) py_block_getattr,
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
  py_block_doc,
};

static PyMethodDef Block_methods[] = {
  {"analysis", py_vorbis_analysis,
   METH_VARARGS, py_vorbis_analysis_doc},
  {NULL, NULL}
};

static void
py_block_dealloc(py_block *self)
{
  vorbis_block_clear(&self->vb);
  Py_XDECREF(self->parent);
  PyMem_DEL(self);
}

static PyObject*
py_block_getattr(PyObject *self, char *name)
{
  return Py_FindMethod(Block_methods, self, name);
}

static PyObject*
py_vorbis_analysis(PyObject *self, PyObject *args)
{
  py_block *b_self = (py_block *) self;
  ogg_packet op;
  vorbis_analysis(&b_self->vb, &op); //TODO error code
  return modinfo->ogg_packet_from_packet(&op);
}

PyObject *
py_block_from_block(vorbis_block *vb, PyObject *parent)
{
  py_block *ret = (py_block *) PyObject_NEW(py_block, 
					    &py_block_type);
  if (ret == NULL)
    return NULL;
  
  ret->vb = *vb;
  ret->parent = parent;
  Py_XINCREF(parent);
  return (PyObject *)ret;
}



