#include <assert.h>
#include <vorbis/vorbisenc.h>

#include "general.h"
#include "vorbismodule.h"

#include "pyvorbisinfo.h"
#include "pyvorbiscodec.h"

/*  
   *********************************************************
                        VorbisInfo Object methods 
   *********************************************************
*/

FDEF(ov_info_clear) "Clears a VorbisInfo object";
FDEF(vorbis_analysis_init) "Create a DSP object to start audio analysis.";

static void py_ov_info_dealloc(py_vinfo *);
static PyObject *py_ov_info_getattr(py_vinfo *, char *name);

static PyMethodDef py_vinfo_methods[] = {
  {"clear", py_ov_info_clear, 
   METH_VARARGS, py_ov_info_clear_doc},
  {"analysis_init", py_vorbis_analysis_init, 
   METH_VARARGS, py_vorbis_analysis_init_doc},
  {NULL, NULL}
};

char py_vinfo_doc[] = 
"A VorbisInfo object stores information about a Vorbis stream.\n\
Information is stored as attributes.";

PyTypeObject py_vinfo_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "VorbisInfo",
  sizeof(py_vinfo),
  0,

  /* Standard Methods */
  (destructor) py_ov_info_dealloc,
  (printfunc) 0,
  (getattrfunc) py_ov_info_getattr,
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
  py_vinfo_doc,
};


PyObject*
py_info_new_from_vi(vorbis_info *vi)
{
  py_vinfo *newobj;
  newobj = (py_vinfo *) PyObject_NEW(py_vinfo, 
				     &py_vinfo_type);
  newobj->vi = *vi;
  return (PyObject *) newobj;
}

static char *py_info_new_kw[] = {"channels", "rate", "max_bitrate",
				 "nominal_bitrate", "min_bitrate", NULL};

PyObject *
py_info_new(PyObject *self, PyObject *args, PyObject *kwdict)
{
  long channels, rate, max_bitrate, nominal_bitrate, min_bitrate;
  vorbis_info vi; 
  int res;

  channels = 2;
  rate = 44100;
  max_bitrate = -1;
  nominal_bitrate = 128000;
  min_bitrate = -1;
  if (!PyArg_ParseTupleAndKeywords(args, kwdict, 
				   "|lllll", py_info_new_kw, 
				   &channels, &rate, &max_bitrate,
				   &nominal_bitrate, &min_bitrate))
    return NULL;
  vorbis_info_init(&vi);

  res = vorbis_encode_init(&vi, channels, rate,
			   max_bitrate, nominal_bitrate,
			   min_bitrate);

  if (res != 0) {
    vorbis_info_clear(&vi);
    v_error_from_code(res, "vorbis_encode_init");
  }

  return py_info_new_from_vi(&vi);
}

static PyObject *
py_ov_info_clear(PyObject *self, PyObject *args)
{
  py_vinfo *ovi_self = (py_vinfo *) self;
  vorbis_info_clear(&ovi_self->vi);

  Py_INCREF(Py_None);
  return Py_None;
}

static void
py_ov_info_dealloc(py_vinfo *self)
{
  PyMem_DEL(self);
}

#define CMP_RET(x) \
   if (strcmp(name, #x) == 0) \
     return PyInt_FromLong(vi->x)

static PyObject *
py_ov_info_getattr(py_vinfo *self, char *name)
{
  PyObject *res;
  vorbis_info *vi = & self->vi;
  char err_msg[MSG_SIZE];

  res = Py_FindMethod(py_vinfo_methods, (PyObject *)self, name);
  if (res)
    return res;
  PyErr_Clear();

  switch(name[0]) {
  case 'b':
    CMP_RET(bitrate_upper);
    CMP_RET(bitrate_nominal);
    CMP_RET(bitrate_lower);
    break;
  case 'c':
    CMP_RET(channels);
    break;
  case 'r':
    CMP_RET(rate);
    break;
  case 'v':
    CMP_RET(version);
    break;
  }

  snprintf(err_msg, MSG_SIZE, "No attribute: %s", name);
  PyErr_SetString(PyExc_AttributeError, err_msg);
  return NULL;
}

static PyObject *
py_vorbis_analysis_init(PyObject *self, PyObject *args)
{
  int res;

  py_vinfo *ovi_self = (py_vinfo *) self;
  vorbis_dsp_state vd;

  if ((res = vorbis_analysis_init(&vd, &ovi_self->vi)))
    return v_error_from_code(res, "vorbis_analysis_init");

  return py_dsp_from_dsp(&vd, self);
}

/*  
   *********************************************************
                   VorbisComment Object methods 
   *********************************************************
*/


FDEF(vorbis_comment_clear) "Clears a VorbisComment object";
FDEF(vorbis_comment_add) "Adds a comment";
FDEF(vorbis_comment_add_tag) "Adds a comment tag";
FDEF(vorbis_comment_query) "Returns a comment_query";
FDEF(vorbis_comment_query_count) "Returns a comment_query_count";

FDEF(comment_as_dict) "Returns a dictionary representation of \
this VorbisComment object";

FDEF(comment_keys) "Returns a list of keys (like a dictionary)";
FDEF(comment_items) "Returns a list of items (like a dictionary).\n\
The list is flattened, so it is a list of strings, not a list of lists.";
FDEF(comment_values) "Returns a list of values (like a dictionary).\n\
The list is flattened, so it is a list of tuples of strings,\n\
not a list of (string, list) tuples.";

static void py_vorbis_comment_dealloc(py_vinfo *);
static PyObject *py_vorbis_comment_getattr(py_vcomment *, char *name);


static PyMethodDef py_vcomment_methods[] = {
  {"clear", py_vorbis_comment_clear, 
   METH_VARARGS, py_vorbis_comment_clear_doc},
  {"add", py_vorbis_comment_add, 
   METH_VARARGS, py_vorbis_comment_add_doc},
  {"add_tag", py_vorbis_comment_add_tag, 
   METH_VARARGS, py_vorbis_comment_add_tag_doc},
  {"query", py_vorbis_comment_query, 
   METH_VARARGS, py_vorbis_comment_query_doc},
  {"query_count", py_vorbis_comment_query_count, 
   METH_VARARGS, py_vorbis_comment_query_count_doc},
  {"as_dict", py_comment_as_dict,
   METH_VARARGS, py_comment_as_dict_doc},
  {"keys", py_comment_keys,
   METH_VARARGS, py_comment_keys_doc},
  {"items", py_comment_items,
   METH_VARARGS, py_comment_items_doc},
  {"values", py_comment_values,
   METH_VARARGS, py_comment_values_doc},
  {NULL, NULL}
};

static int py_comment_length(py_vcomment *);
static int py_comment_assign(py_vcomment *, 
				   PyObject *, PyObject *);
static PyObject *py_comment_subscript(py_vcomment *, PyObject *);

static PyMappingMethods py_vcomment_Mapping_Methods = {
  (inquiry) py_comment_length,
  (binaryfunc) py_comment_subscript,
  (objobjargproc) py_comment_assign
};

char py_vcomment_doc[] =
"A VorbisComment object stores comments about a Vorbis stream.\n\
It is used much like a Python dictionary. The keys are case-insensitive,\n\
and each value will be a list of one or more values, since keys in a\n\
Vorbis stream's comments do not have to be unique.\n\
\n\
\"mycomment[key] = val\" will append val to the list of values for key\n\
A comment object also has the keys() items() and values() functions that\n\
dictionaries have, the difference being that the lists in items() and\n\
values() are flattened. So if there are two 'Artist' entries, there will\n\
be two separate tuples in items() for 'Artist' and two strings for 'Artist'\n\
in value().";

PyTypeObject py_vcomment_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "VorbisComment",
  sizeof(py_vcomment),
  0,

  /* Standard Methods */
  (destructor) py_vorbis_comment_dealloc,
  (printfunc) 0,
  (getattrfunc) py_vorbis_comment_getattr,
  (setattrfunc) 0,
  (cmpfunc) 0,
  (reprfunc) 0,
  
  /* Type Categories */
  0, /* as number */
  0, /* as sequence */
  &py_vcomment_Mapping_Methods,
  0, /* hash */
  0, /* binary */
  0, /* repr */
  0, /* getattro */
  0, /* setattro */
  0, /* as buffer */
  0, /* tp_flags */
  py_vcomment_doc,
};



// I store the parent since I don't think the vorbis_comment data will actually
// stick around if we let the vorbis_file object get freed.

PyObject *
py_comment_new_from_vc(vorbis_comment *vc, PyObject *parent)
{
  py_vcomment *newobj;

  newobj = (py_vcomment *) PyObject_NEW(py_vcomment, 
					&py_vcomment_type);
  newobj->vc = *vc;
  newobj->parent = parent;
  Py_XINCREF(parent);
  return (PyObject *) newobj;
}

PyObject *
py_comment_new(PyObject *self, PyObject *args)
{
  py_vcomment *newobj;

  newobj = (py_vcomment *) PyObject_NEW(py_vcomment, 
					&py_vcomment_type);
  vorbis_comment_init(&newobj->vc);
  newobj->parent = NULL;
  return (PyObject *) newobj;
}

static PyObject *
py_vorbis_comment_clear(PyObject *self, PyObject *args)
{
  py_vcomment *ovc_self = (py_vcomment *) self;

  vorbis_comment_clear(&ovc_self->vc);
  vorbis_comment_init(&ovc_self->vc);

  Py_INCREF(Py_None);
  return Py_None;
}

static void
py_vorbis_comment_dealloc(py_vinfo *self)
{
  py_vcomment *ovc_self = (py_vcomment *) self;

  if (ovc_self->parent)
    Py_DECREF(ovc_self->parent); //parent will clear for us
  else
    vorbis_comment_clear(&ovc_self->vc); 

  PyMem_DEL(self);
}

static PyObject*
py_vorbis_comment_getattr(py_vcomment *self, char *name)
{
  PyObject *res;

  res = Py_FindMethod(py_vcomment_methods, (PyObject *) self, name);
  return res;
}

static int
py_comment_length(py_vcomment *self)
{
  int val = self->vc.comments;
  if (self->vc.vendor) val++;
  return val;
}

static PyObject *
py_comment_subscript(py_vcomment *self, 
		     PyObject *keyobj)
{
  char *res, *tag;
  int cur = 0;
  PyObject *retlist, *item;

  if (!PyString_Check(keyobj)) {
    PyErr_SetString(PyExc_KeyError, "Keys may only be strings");
    return NULL;
  }

  tag = PyString_AsString(keyobj);
  retlist = PyList_New(0);

  res = vorbis_comment_query(&self->vc, tag, cur++);
  while (res != NULL) {
    int vallen = strlen(res);
#if PY_UNICODE
    item = PyUnicode_DecodeUTF8(res, vallen, NULL);
#else
    item = PyString_FromStringAndSize(res, vallen);
#endif
    PyList_Append(retlist, item);
    Py_DECREF(item);
    
    res = vorbis_comment_query(&self->vc, tag, cur++);
  }

  if (cur == 1) {
    Py_DECREF(retlist);
    PyErr_SetString(PyExc_KeyError, "Key not found");
    return NULL;
  }
  return retlist;
}

static int
py_comment_assign(py_vcomment *self, 
		  PyObject *keyobj, PyObject *valobj)
{
  vorbis_comment *vc = &self->vc;
  char *tag, *val;

  if (!PyString_Check(keyobj)) {
    PyErr_SetString(PyExc_KeyError, "Keys may only be ASCII strings");
    return -1;
  }

  if (PyString_Check(valobj)) {
    val = PyString_AsString(valobj);
  } 
#if PY_UNICODE
  else if (PyUnicode_Check(valobj)) {
    PyObject *unistring = PyUnicode_AsUTF8String(valobj);
    val = PyString_AsString(unistring);
    Py_DECREF(unistring);
  } 
#endif
  else {
    PyErr_SetString(PyExc_KeyError, "Values may only be strings");
    return -1;
  }

  tag = PyString_AsString(keyobj);

  vorbis_comment_add_tag(vc, tag, val);
  return 0;
}

static PyObject *
py_comment_keys(PyObject *self, PyObject *args)
{
  PyObject *dict = py_comment_as_dict(self, NULL);
  PyObject *keys;
  
  if (!dict)
    return NULL;
  keys = PyDict_Keys(dict);
  Py_DECREF(dict);
  return keys;
}

static PyObject *
py_comment_items(PyObject *self, PyObject *args)
{
  int curitem, curpos, j;
  PyObject *key, *val, *curval, *tuple;
  PyObject *retlist = PyList_New(0);
  PyObject *dict = py_comment_as_dict(self, NULL);

  curitem = curpos = 0;

  while (PyDict_Next(dict, &curitem, &key, &val) > 0) {
    assert(PyList_Check(val));
    //flatten out the list
    for (j = 0; j < PyList_Size(val); j++) {
      tuple = PyTuple_New(2);

      curval = PyList_GetItem(val, j);
      Py_INCREF(key);
      Py_INCREF(curval);

      PyTuple_SET_ITEM(tuple, 0, key);
      PyTuple_SET_ITEM(tuple, 1, curval);
      PyList_Append(retlist, tuple);
      Py_DECREF(tuple);
    }
  }
  Py_DECREF(dict);
  return retlist;
}

static PyObject *
py_comment_values(PyObject *self, PyObject *args)
{
  int curitem, curpos, j;
  PyObject *key, *val, *curval;
  PyObject *retlist = PyList_New(0);
  PyObject *dict = py_comment_as_dict(self, NULL);

  curitem = curpos = 0;

  while (PyDict_Next (dict, &curitem, &key, &val)) {
    assert(PyList_Check(val));
    //flatten out the list
    for (j = 0; j < PyList_Size(val); j++) {
      curval = PyList_GET_ITEM(val, j);
      PyList_Append(retlist, curval);
    }
  }

  Py_DECREF(dict);
  return retlist;
}

static PyObject *
py_vorbis_comment_add(PyObject *self, PyObject *args)
{
  py_vcomment *ovc_self = (py_vcomment *) self;
  char *comment;

  if (!PyArg_ParseTuple(args, "s", &comment))
    return NULL;
  
  vorbis_comment_add(&ovc_self->vc, comment);
  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_vorbis_comment_add_tag(PyObject *self, PyObject *args)
{
  py_vcomment *ovc_self = (py_vcomment *) self;
  char *comment, *tag;

  // TODO: What will this store if it's unicode? I think UTF-16, want UTF-8.
  // TODO: Learn Unicode!!
  if (!PyArg_ParseTuple(args, "ss", &comment, &tag))
    return NULL;

  vorbis_comment_add_tag(&ovc_self->vc, comment, tag);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
py_vorbis_comment_query(PyObject *self, PyObject *args)
{
  char *tag, *res;
  int count;
  vorbis_comment *vc = &((py_vcomment *) self)->vc;

  if (!PyArg_ParseTuple(args, "si", &tag, &count))
    return NULL;

  res = vorbis_comment_query(vc, tag, count);
  return PyString_FromString(res);
}

static PyObject *
py_vorbis_comment_query_count(PyObject *self, PyObject *args)
{
  char *tag;
  vorbis_comment *vc = &((py_vcomment *) self)->vc;

  if (!PyArg_ParseTuple(args, "s", &tag))
    return NULL;

  return PyInt_FromLong(vorbis_comment_query_count(vc, tag));
}

static int
make_caps_key(char *in, int size)
{
  int pos; 
  for (pos = 0; pos < size && in[pos] != '\0'; pos++) {
    if (in[pos] >= 'a' && in[pos] <= 'z')
      in[pos] = in[pos] + 'A' - 'a';
    else 
      in[pos] = in[pos];
  }
  in[pos] = '\0';
  return 0;
}

static PyObject *
py_comment_as_dict(PyObject *self, PyObject *args)
{
  vorbis_comment *comment;
  py_vcomment *ovc_self = (py_vcomment *) self;

  int i, keylen, vallen;
  char *key, *val;

  PyObject *retdict, *curlist, *item, *vendor_obj;
  
  comment = &ovc_self->vc;
  retdict = PyDict_New();


  // If the vendor is set, set the key "VENDOR" to map to a 
  // singleton list with the vendor value in it.

  if (comment->vendor != NULL) {
    curlist = PyList_New(1);
    vendor_obj = PyString_FromString(comment->vendor);
    PyList_SET_ITEM(curlist, 0, vendor_obj);
    PyDict_SetItemString(retdict, "VENDOR", curlist);
    Py_DECREF(curlist);
  } else 
    vendor_obj = NULL;

  //These first few lines taken from the Perl bindings
  for (i = 0; i < comment->comments; i++) {
    //don't want to limit this, I guess
    key = strdup(comment->user_comments[i]); 
    
    if ((val = strchr(key, '='))) {
      keylen = val - key;
      *(val++) = '\0';
      vallen = comment->comment_lengths[i] - keylen - 1;
      
#if PY_UNICODE
      item = PyUnicode_DecodeUTF8(val, vallen, NULL);
#else
      item = PyString_FromStringAndSize(val, vallen);
#endif

      if (!item)
	goto error;
      
      if (make_caps_key(key, keylen)) { // overwrites key
	Py_DECREF(item);
	goto error;
      }

      // GetItem borrows a reference
      if ((curlist = PyDict_GetItemString(retdict, key))) {

	// A list already exists for that key
	if (PyList_Append(curlist, item) < 0) {
	  Py_DECREF(item);
	  goto error;
	}

      } else {

	// Add a new list in that position
	curlist = PyList_New(1);
	PyList_SET_ITEM(curlist, 0, item);
	Py_INCREF(item);

	//this does not steal a reference
	PyDict_SetItemString(retdict, key, curlist); 
	Py_DECREF(curlist);
      }
      Py_DECREF(item);
    }
    free(key);
    key = NULL;
  }
  return retdict;
 error:
  Py_XDECREF(retdict);
  if (key)
    free(key);
  return NULL;
}
























