#include <Python.h>
#include <vorbis/vorbisfile.h>

static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    PyObject *file, *read;
    PyObject *arglist, *result;
    char *str;
    size_t strsize;

    file = (PyObject *)datasource;
    read = PyObject_GetAttrString(file, "read");

    arglist = Py_BuildValue("(i)", size*nmemb);
    result = PyEval_CallObject(read, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
	printf("DEBUG: got null result from read()\n");
	PyErr_Print();
	return -1;
    }

    if (!PyString_Check(result)) {
	printf("DEBUG: result is not a string!\n");
	PyErr_SetString(PyExc_StandardError, "Result of read callback not a string");
	return -1;
    }


    strsize = PyString_Size(result);
    str = PyString_AsString(result);
    memcpy(ptr, (void *)str, strsize);

    Py_DECREF(result);

    return strsize;
}

static int seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    PyObject *file, *seek, *result;
    PyObject *arglist;

    file = (PyObject *)datasource;
    seek = PyObject_GetAttrString(file, "seek");

    /* call seek func */
    arglist = Py_BuildValue("(li)", (long)offset, whence);
    result = PyEval_CallObject(seek, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
	printf("DEBUG: got null result from seek\n");
	PyErr_Print();
	return -1;
    }

    Py_DECREF(result);

    return 0;
}

static int close_func(void *datasource)
{
    PyObject *file, *close;
    PyObject *result;

    file = (PyObject *)datasource;
    close = PyObject_GetAttrString(file, "close");
    
    result = PyEval_CallObject(close, NULL);
    if (result != NULL) Py_DECREF(result);
    
    Py_DECREF(file);
    
    return 0;
}

static long tell_func(void *datasource)
{
    PyObject *file, *tell;
    PyObject *result;
    long ret;

    file = (PyObject *)datasource;
    tell = PyObject_GetAttrString(file, "tell");

    result = PyEval_CallObject(tell, NULL);

    if (result == NULL) {
	printf("DEBUG: got null result from tell\n");
	PyErr_Print();
	return -1;
    }

    if (PyLong_Check(result)) {
	ret = PyLong_AsLong(result);
    } else if (PyInt_Check(result)) {
	ret = PyInt_AsLong(result);
    } else {
	printf("DEBUG: got non-long/non-int from tell\n");
	return -1;
    }

    return ret;
}

static void vf_destroy(void *data)
{
    ov_clear((OggVorbis_File *)data);
}

/* ov_open_py(file)
** returns a vorbisfile object
*/
static PyObject *ov_open_py(PyObject *self, PyObject *args)
{
    PyObject *file, *read, *seek, *close, *tell;
    PyObject *result;
    ov_callbacks callbacks;
    OggVorbis_File *vf;
    int ret;
    
    if (!PyArg_ParseTuple(args, "O", &file)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyFile_Check(file)) {
	PyErr_SetString(PyExc_TypeError, "Expected a file object");
	return NULL;
    }

    /* setup callback functions */
    callbacks.read_func = read_func;
    callbacks.seek_func = seek_func;
    callbacks.tell_func = tell_func;
    callbacks.close_func = close_func;


    vf = (OggVorbis_File *)PyMem_Malloc(sizeof(OggVorbis_File));
    Py_INCREF(file);
    ret = ov_open_callbacks((void *)file, vf, NULL, 0, callbacks);
    if (ret != 0) {
	/* FIXME: handle open failures more gracefully */
	PyErr_SetString(PyExc_StandardError, "ov_open failed");
	PyMem_Free((void *)vf);
	return NULL;
    }

    result = PyCObject_FromVoidPtr((void *)vf, vf_destroy);

    return result;
}

static PyObject *ov_test_py(PyObject *self, PyObject *args)
{
    PyObject *file, *read, *seek, *close, *tell;
    PyObject *result;
    ov_callbacks callbacks;
    OggVorbis_File *vf;
    int ret;
    
    if (!PyArg_ParseTuple(args, "O", &file)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyFile_Check(file)) {
	PyErr_SetString(PyExc_TypeError, "Expected a file object");
	return NULL;
    }

    /* setup callback functions */
    callbacks.read_func = read_func;
    callbacks.seek_func = seek_func;
    callbacks.tell_func = tell_func;
    callbacks.close_func = close_func;


    vf = (OggVorbis_File *)PyMem_Malloc(sizeof(OggVorbis_File));
    Py_INCREF(file);
    ret = ov_test_callbacks((void *)file, vf, NULL, 0, callbacks);
    if (ret != 0) {
	/* FIXME: implement error handling */
	PyErr_SetString(PyExc_StandardError, "ov_test failed");
	PyMem_Free((void *)vf);
	return NULL;
    }

    result = PyCObject_FromVoidPtr((void *)vf, vf_destroy);

    return result;
}

static PyObject *ov_test_open_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_TypeError, "Expected a vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_test_open(vf);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_clear_py(PyObject *self, PyObject *args)
{
    PyObject *cobj;
    OggVorbis_File *vf;
    int ret;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_clear(vf);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_bitrate_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int link;
    long ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);

    ret = ov_bitrate(vf, link);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_bitrate_instant_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    long ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_bitrate_instant(vf);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_streams_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    long ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_streams(vf);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_seekable_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    long ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_seekable(vf);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_serialnumber_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    long ret;
    int link;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_serialnumber(vf, link);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_raw_total_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    ogg_int64_t ret;
    int link;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_raw_total(vf, link);

    result = PyLong_FromLongLong(ret);
    return result;
}

static PyObject *ov_pcm_total_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    ogg_int64_t ret;
    int link;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_pcm_total(vf, link);

    result = PyLong_FromLongLong(ret);
    return result;
}

static PyObject *ov_time_total_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    double ret;
    int link;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_time_total(vf, link);

    result = Py_BuildValue("d", ret);
    return result;
}

static PyObject *ov_raw_seek_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    ogg_int64_t pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "OL", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_raw_seek(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_pcm_seek_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    ogg_int64_t pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "OL", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_pcm_seek(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_pcm_seek_page_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    ogg_int64_t pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "OL", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_pcm_seek_page(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_time_seek_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    double pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Od", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_time_seek(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_time_seek_page_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    double pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Od", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_time_seek_page(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_raw_seek_lap_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    ogg_int64_t pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "OL", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_raw_seek_lap(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_pcm_seek_lap_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    ogg_int64_t pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "OL", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_pcm_seek_lap(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_pcm_seek_page_lap_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    ogg_int64_t pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "OL", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_pcm_seek_page_lap(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_time_seek_lap_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    double pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Od", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_time_seek_lap(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_time_seek_page_lap_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    double pos;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Od", &cobj, &pos)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_time_seek_page_lap(vf, pos);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_raw_tell_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    ogg_int64_t ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_raw_tell(vf);

    result = PyLong_FromLongLong(ret);
    return result;
}

static PyObject *ov_pcm_tell_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    ogg_int64_t ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_pcm_tell(vf);

    result = PyLong_FromLongLong(ret);
    return result;
}

static PyObject *ov_time_tell_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    double ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_time_tell(vf);

    result = Py_BuildValue("d", ret);
    return result;
}

static PyObject *ov_info_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    vorbis_info *ret;
    int link;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_info(vf, link);

    result = PyCObject_FromVoidPtr((void *)ret, NULL);
    return result;
}

static PyObject *ov_comment_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    vorbis_comment *ret;
    int link;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &link)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_comment(vf, link);

    result = PyCObject_FromVoidPtr((void *)ret, NULL);
    return result;
}

static PyObject *ov_read_float_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result, *channel[255], *list;
    long ret;
    int current_section, samples, channels, i, j;
    float **pcm_channels;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &samples)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    channels = vf->vi->channels;

    ret = ov_read_float(vf, &pcm_channels, samples, &current_section);
    if (ret > 0) {
	list = PyList_New(channels);
	for (i=0; i<channels; i++) {
	    channel[i] = PyList_New(ret);
	    PyList_SetItem(list, i, channel[i]);
	    for (j=0; j<ret; j++) {
		PyList_SetItem(channel[i], j, 
			       PyFloat_FromDouble(pcm_channels[i][j]));
	    }
	}
    }

    result = Py_BuildValue("(lOi)", ret, list, current_section);
    return result;
}

static PyObject *ov_read_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    long ret;
    int current_section, len, bigendianp, word, sgned;
    OggVorbis_File *vf;
    char *buffer;

    if (!PyArg_ParseTuple(args, "Oiiii", &cobj, &len, &bigendianp, &word, 
			  &sgned)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    buffer = (char *)PyMem_Malloc(len);
    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_read(vf, buffer, len, bigendianp, word, sgned, &current_section);
    if (ret > 0)
	buffer = (char *)PyMem_Realloc(buffer, ret);

    result = Py_BuildValue("(ls#i)", ret, buffer, ret, current_section);
    return result;
}

static PyObject *ov_crosslap_py(PyObject *self, PyObject *args)
{
    PyObject *cobj1, *cobj2, *result;
    int ret;
    OggVorbis_File *vf1, *vf2;

    if (!PyArg_ParseTuple(args, "OO", &cobj1, &cobj2)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj1)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    if (!PyCObject_Check(cobj2)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf1 = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj1);
    vf2 = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj2);
    ret = ov_crosslap(vf1, vf2);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_halfrate_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret, flag;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "Oi", &cobj, &flag)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_halfrate(vf, flag);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyObject *ov_halfrate_p_py(PyObject *self, PyObject *args)
{
    PyObject *cobj, *result;
    int ret;
    OggVorbis_File *vf;

    if (!PyArg_ParseTuple(args, "O", &cobj)) {
	PyErr_SetString(PyExc_StandardError, "Couldn't parse arguments");
	return NULL;
    }

    if (!PyCObject_Check(cobj)) {
	PyErr_SetString(PyExc_StandardError, "Expected vorbisfile object");
	return NULL;
    }

    vf = (OggVorbis_File *)PyCObject_AsVoidPtr(cobj);
    ret = ov_halfrate_p(vf);

    result = Py_BuildValue("i", ret);
    return result;
}

static PyMethodDef vorbisfileMethods[] = {
    {"ov_open", ov_open_py, METH_VARARGS, 
     "Opens a Vorbis file"},
    {"ov_test", ov_test_py, METH_VARARGS,
     "Tests if a file is a Vorbis file"},
    {"ov_test_open", ov_test_open_py, METH_VARARGS,
     "Opens a Vorbis file after testing"},
    {"ov_clear", ov_clear_py, METH_VARARGS, 
     "Clears a vorbisfile object"},
    {"ov_bitrate", ov_bitrate_py, METH_VARARGS, 
     "Returns bitrate of chain(s)"},
    {"ov_bitrate_instant", ov_bitrate_instant_py, METH_VARARGS,
     "Returns instantaneous bitrate"},
    {"ov_streams", ov_streams_py, METH_VARARGS, 
     "Returns number of chains"},
    {"ov_seekable", ov_seekable_py, METH_VARARGS, 
     "Returns whether stream is seekable"},
    {"ov_serialnumber", ov_serialnumber_py, METH_VARARGS,
     "Returns a link's serial number"},
    {"ov_raw_total", ov_raw_total_py, METH_VARARGS, 
     "Returns total raw compressed length of bitstream"},
    {"ov_pcm_total", ov_pcm_total_py, METH_VARARGS, 
     "Returns total number of PCM samples"},
    {"ov_time_total", ov_time_total_py, METH_VARARGS, 
     "Returns total length of bitstream in seconds"},
    {"ov_raw_seek", ov_raw_seek_py, METH_VARARGS, 
     "Seeks to an offset relative to compressed data"},
    {"ov_pcm_seek", ov_pcm_seek_py, METH_VARARGS, 
     "Seeks to a sample offset of the PCM data"},
    {"ov_pcm_seek_page", ov_pcm_seek_page_py, METH_VARARGS,
     "Seeks to page near sample offset"},
    {"ov_time_seek", ov_time_seek_py, METH_VARARGS,
     "Seeks to sample at time"},
    {"ov_time_seek_page", ov_time_seek_page_py, METH_VARARGS,
     "Seeks to page near sample at time"},
    {"ov_raw_seek_lap", ov_raw_seek_lap_py, METH_VARARGS,
     "Seeks something (FIXME)"},
    {"ov_pcm_seek_lap", ov_pcm_seek_lap_py, METH_VARARGS,
     "Seeks something (FIXME)"},
    {"ov_pcm_seek_page_lap", ov_pcm_seek_page_lap_py, METH_VARARGS,
     "Seeks something (FIXME)"},
    {"ov_time_seek_lap", ov_time_seek_lap_py, METH_VARARGS,
     "Seeks something (FIXME)"},
    {"ov_time_seek_page_lap", ov_time_seek_page_lap_py, METH_VARARGS,
     "Seeks something (FIXME)"},
    {"ov_raw_tell", ov_raw_tell_py, METH_VARARGS, 
     "Returns the current stream offset"},
    {"ov_pcm_tell", ov_pcm_tell_py, METH_VARARGS,
     "Returns PCM offset of next sample to be read"},
    {"ov_time_tell", ov_time_tell_py, METH_VARARGS,
     "Returns time offset of next sample to be read"},
    {"ov_info", ov_info_py, METH_VARARGS,
     "Returns the vorbis_info object for the current chain"},
    {"ov_comment", ov_comment_py, METH_VARARGS,
     "Returns the vorbis_comment object for the current chain"},
    {"ov_read_float", ov_read_float_py, METH_VARARGS,
     "Returns array of decoded float data"},
    {"ov_read", ov_read_py, METH_VARARGS,
     "Returns array of decoded sample data"},
    {"ov_crosslap", ov_crosslap_py, METH_VARARGS,
     "Crosses something (FIXME)"},
    {"ov_halfrate", ov_halfrate_py, METH_VARARGS,
     "Does something (FIXME)"},
    {"ov_halfrate_p", ov_halfrate_p_py, METH_VARARGS,
     "Does something (FIXME)"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_vorbisfile(void)
{
    Py_InitModule("_vorbisfile", vorbisfileMethods);
}
