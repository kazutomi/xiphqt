#include "pyoggpackbuff.h"
#include "pyoggpacket.h"
#include "general.h"
#include "_ogg2module.h"

/************************************************************
			 OggPackBuffer Object
************************************************************/

char PyOggPackBuffer_Doc[] = "";

static void PyOggPackBuffer_Dealloc(PyObject *);
static PyObject* PyOggPackBuffer_Getattr(PyObject *, char *);
static PyObject *PyOggPackBuffer_Repr(PyObject *self);

FDEF(PyOggPackBuffer_Bytes) "Return the number of bytes in the buffer";
FDEF(PyOggPackBuffer_Bits) "Return the number of bits in the buffer";

FDEF(PyOggPackBuffer_Read) "Return the value of n bits";
FDEF(PyOggPackBuffer_Look) "Return the value of n bits without advancing pointer";
FDEF(PyOggPackBuffer_Adv) "Advance the read location by n bits";

FDEF(PyOggPackBuffer_Reset) "Clears and resets the packet buffer";
FDEF(PyOggPackBuffer_Packetout) "Export the OggPacket built by the buffer";
FDEF(PyOggPackBuffer_Write) "Write bits to the buffer.\n\n\
The first parameter is an integer from which the bits will be extracted.\n\
The second parameter is the number of bits to write (defaults to 1)";

PyTypeObject PyOggPackBuffer_Type = {
  PyObject_HEAD_INIT(NULL)
  0,
  "Oggpack_Buffer",
  sizeof(PyOggPackBufferObject),
  0,
  
  /* Standard Methods */
  /* (destructor) */ PyOggPackBuffer_Dealloc,
  /* (printfunc) */ 0,
  /* (getattrfunc) */ PyOggPackBuffer_Getattr,
  /* (setattrfunc) */ 0,
  /* (cmpfunc) */ 0,
  /* (reprfunc) */ PyOggPackBuffer_Repr,
  
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
  PyOggPackBuffer_Doc
};

static PyMethodDef PyOggPackBuffer_Read_Methods[] = {
  {"bytes", PyOggPackBuffer_Bytes,
   METH_VARARGS, PyOggPackBuffer_Bytes_Doc},
  {"bits", PyOggPackBuffer_Bits,
   METH_VARARGS, PyOggPackBuffer_Bits_Doc},
  {"look", PyOggPackBuffer_Look,
   METH_VARARGS, PyOggPackBuffer_Look_Doc},
  {"read", PyOggPackBuffer_Read,
   METH_VARARGS, PyOggPackBuffer_Read_Doc},
  {"adv", PyOggPackBuffer_Adv,
   METH_VARARGS, PyOggPackBuffer_Adv_Doc},
  {NULL, NULL}  
};

static PyMethodDef PyOggPackBuffer_Write_Methods[] = {
  {"bytes", PyOggPackBuffer_Bytes,
   METH_VARARGS, PyOggPackBuffer_Bytes_Doc},
  {"bits", PyOggPackBuffer_Bits,
   METH_VARARGS, PyOggPackBuffer_Bits_Doc},
  {"reset", PyOggPackBuffer_Reset,
   METH_VARARGS, PyOggPackBuffer_Reset_Doc},
  {"write", PyOggPackBuffer_Write,
   METH_VARARGS, PyOggPackBuffer_Write_Doc},
  {"packetout", PyOggPackBuffer_Packetout,
   METH_VARARGS, PyOggPackBuffer_Packetout_Doc},
  {NULL, NULL}  
};


PyObject *
PyOggPackBuffer_New(PyObject *self, PyObject *args) 
{
  PyOggPacketObject *packetobj;
  PyOggPackBufferObject *ret;

  packetobj = NULL;

  if ( !PyArg_ParseTuple(args, "|O!", &PyOggPacket_Type,
                         (PyObject *) &packetobj) ) return NULL;

  ret = (PyOggPackBufferObject *) PyObject_NEW(PyOggPackBufferObject,
                                               &PyOggPackBuffer_Type);
  if (ret == NULL) return NULL;
  ret->msb_flag = 0;
  PyOggPackBuffer_AsOggPackBuffer(ret) = PyMem_Malloc(oggpack_buffersize());

  if ( packetobj ) { 
    ret->write_flag = 0;
    ret->packetobj = packetobj; /* Must keep packet around for now! */
    Py_INCREF(((PyOggPackBufferObject *) ret)->packetobj);
    oggpack_readinit(PyOggPackBuffer_AsOggPackBuffer(ret), 
                     PyOggPacket_AsOggPacket(packetobj)->packet);
    return (PyObject *)ret;
  } 
  ret->write_flag = 1;
  oggpack_writeinit(PyOggPackBuffer_AsOggPackBuffer(ret), 
                    ogg_buffer_create());
  return (PyObject *)ret;
}


PyObject *
PyOggPackBuffer_NewB(PyObject *self, PyObject *args) 
{
  PyOggPacketObject *packetobj;
  PyOggPackBufferObject *ret;

  packetobj = NULL;

  if ( !PyArg_ParseTuple(args, "|O!", &PyOggPacket_Type,
                         (PyObject *) &packetobj) ) return NULL;

  ret = (PyOggPackBufferObject *) PyObject_NEW(PyOggPackBufferObject,
                                               &PyOggPackBuffer_Type);
  if (ret == NULL) return NULL;
  ret->msb_flag = 1;
  PyOggPackBuffer_AsOggPackBuffer(ret) = PyMem_Malloc(oggpack_buffersize());

  if ( packetobj ) { 
    ret->write_flag = 0;
    ret->packetobj = packetobj; /* Must keep packet around for now! */
    Py_INCREF(((PyOggPackBufferObject *) ret)->packetobj);
    oggpack_readinit(PyOggPackBuffer_AsOggPackBuffer(ret), 
                     PyOggPacket_AsOggPacket(packetobj)->packet);
    return (PyObject *)ret;
  } 
  ret->write_flag = 1;
  oggpack_writeinit(PyOggPackBuffer_AsOggPackBuffer(ret), 
                    ogg_buffer_create());
  return (PyObject *)ret;
}


static void
PyOggPackBuffer_Dealloc(PyObject *self)
{
  if ( ((PyOggPackBufferObject *) self)->write_flag ) { 
    if ( ((PyOggPackBufferObject *) self)->write_flag == 1 ) {
      if ( ((PyOggPackBufferObject *) self)->msb_flag )
        oggpackB_writeclear(PyOggPackBuffer_AsOggPackBuffer(self));
      else 
        oggpack_writeclear(PyOggPackBuffer_AsOggPackBuffer(self));
    }
  }
  else  /* Release the packet being read */
    Py_DECREF(((PyOggPackBufferObject *) self)->packetobj); 
  PyMem_Free(PyOggPackBuffer_AsOggPackBuffer(self));
  PyObject_DEL(self);
}

static PyObject*
PyOggPackBuffer_Getattr(PyObject *self, char *name)
{
  if ( !((PyOggPackBufferObject *) self)->write_flag ) 
    return Py_FindMethod(PyOggPackBuffer_Read_Methods, self, name);
  else {
    if ( ((PyOggPackBufferObject *) self)->write_flag == 2) {
      PyErr_SetString(PyExc_ValueError, "DEAD BUFFER!");
      return NULL;
    }
    return Py_FindMethod(PyOggPackBuffer_Write_Methods, self, name);
  }
}


static PyObject *
PyOggPackBuffer_Bytes(PyObject *self, PyObject *args)
{
  long ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  if ( ((PyOggPackBufferObject *) self)->msb_flag )
    ret = oggpackB_bytes(PyOggPackBuffer_AsOggPackBuffer(self));
  else
    ret = oggpack_bytes(PyOggPackBuffer_AsOggPackBuffer(self));
  return PyLong_FromLong(ret);
}

static PyObject *
PyOggPackBuffer_Bits(PyObject *self, PyObject *args)
{
  long ret;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  if ( ((PyOggPackBufferObject *) self)->msb_flag )
    ret = oggpackB_bits(PyOggPackBuffer_AsOggPackBuffer(self));
  else
    ret = oggpack_bits(PyOggPackBuffer_AsOggPackBuffer(self));
  return PyLong_FromLong(ret);
}


static PyObject *
PyOggPackBuffer_Reset(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
    
  if ( ((PyOggPackBufferObject *) self)->msb_flag )
    oggpackB_writeclear(PyOggPackBuffer_AsOggPackBuffer(self));
  else
    oggpack_writeclear(PyOggPackBuffer_AsOggPackBuffer(self));
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggPackBuffer_Look(PyObject *self, PyObject *args) 
{
  int bits = 1;
  long num;
  long ret;
  if (!PyArg_ParseTuple(args, "l", &bits))
    return NULL;

  if ( bits == 1 ) {
    if ( ((PyOggPackBufferObject *) self)->msb_flag )
      ret = oggpackB_look1(PyOggPackBuffer_AsOggPackBuffer(self));
    else
      ret = oggpack_look1(PyOggPackBuffer_AsOggPackBuffer(self));
    return PyLong_FromLong(ret);
  }
  
  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot look at more than 32 bits");
    return NULL;
  }

  if ( ((PyOggPackBufferObject *) self)->msb_flag )
    ret = oggpackB_look(PyOggPackBuffer_AsOggPackBuffer(self), bits, &num);
  else
    ret = oggpack_look(PyOggPackBuffer_AsOggPackBuffer(self), bits, &num);
  if ( ret == 0 )
    return PyLong_FromLong(num);
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggPackBuffer_Read(PyObject *self, PyObject *args) 
{
  int bits = 1;
  long num;
  long ret;
  
  if (!PyArg_ParseTuple(args, "|i", &bits))
    return NULL;
  
  if ( bits == 1 ) {
    if ( ((PyOggPackBufferObject *) self)->msb_flag )
      ret = oggpackB_read1(PyOggPackBuffer_AsOggPackBuffer(self));
    else
      ret = oggpack_read1(PyOggPackBuffer_AsOggPackBuffer(self));
    return PyInt_FromLong(ret);
  }
  
  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot read more than 32 bits");
    return NULL;
  } 


  if ( ((PyOggPackBufferObject *) self)->msb_flag )
    ret = oggpackB_read(PyOggPackBuffer_AsOggPackBuffer(self), bits, &num);
  else
    ret = oggpack_read(PyOggPackBuffer_AsOggPackBuffer(self), bits, &num);
  if ( ret == 0 )
    return PyInt_FromLong(num);
  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggPackBuffer_Adv(PyObject *self, PyObject *args)
{
  int bits = 1;

  if (!PyArg_ParseTuple(args, "|i", &bits))
    return NULL;

  if ( bits == 1 ) 
    if ( ((PyOggPackBufferObject *) self)->msb_flag )
      oggpackB_adv1(PyOggPackBuffer_AsOggPackBuffer(self));
    else
      oggpack_adv1(PyOggPackBuffer_AsOggPackBuffer(self));
  else 
    if ( ((PyOggPackBufferObject *) self)->msb_flag )
      oggpackB_adv(PyOggPackBuffer_AsOggPackBuffer(self), bits);
    else
      oggpack_adv(PyOggPackBuffer_AsOggPackBuffer(self), bits);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggPackBuffer_Write(PyObject *self, PyObject *args) 
{
  long val;
  int bits = 32;

  if (!PyArg_ParseTuple(args, "l|l", &val, &bits)) 
    return NULL;

  if (bits > 32) {
    PyErr_SetString(PyExc_ValueError, "Cannot write more than 32 bits");
    return NULL;
  }
    
  if ( ((PyOggPackBufferObject *) self)->msb_flag )
    oggpackB_write(PyOggPackBuffer_AsOggPackBuffer(self), val, bits);
  else
    oggpack_write(PyOggPackBuffer_AsOggPackBuffer(self), val, bits);

  Py_INCREF(Py_None);
  return Py_None;
}


static PyObject *
PyOggPackBuffer_Packetout(PyObject *self, PyObject *args)
{
  ogg_packet *op;
  PyOggPacketObject *packetobj;

  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  packetobj = PyOggPacket_Alloc();
  op = packetobj->packet;

  if ( ((PyOggPackBufferObject *) self)->msb_flag ) {
    op->packet = oggpackB_writebuffer(PyOggPackBuffer_AsOggPackBuffer(self));
    op->bytes = oggpackB_bytes(PyOggPackBuffer_AsOggPackBuffer(self));
  } else {
    op->packet = oggpackB_writebuffer(PyOggPackBuffer_AsOggPackBuffer(self));
    op->bytes = oggpackB_bytes(PyOggPackBuffer_AsOggPackBuffer(self));
  }
  op->b_o_s = 0;
  op->e_o_s = 0;
  op->granulepos = 0;
  op->packetno = 0;

  ((PyOggPackBufferObject *) self)->write_flag = 2;
  return (PyObject *) packetobj;
}
  

static PyObject *
PyOggPackBuffer_Repr(PyObject *self)
{
  oggpack_buffer *ob = PyOggPackBuffer_AsOggPackBuffer(self);
  char buf[256];

  if ( ((PyOggPackBufferObject *) self)->write_flag == 2 ) {
    PyErr_SetString(PyExc_ValueError, "DEAD BUFFER!");
    return NULL;
  }
  sprintf(buf, "<OggPackBuff at %p>", self);
  return PyString_FromString(buf);
}

