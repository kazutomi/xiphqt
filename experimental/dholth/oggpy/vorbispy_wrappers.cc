/* Wrappers for vorbispy.
 *
 * Daniel Holth <dholth@fastmail.fm>
 * 21 Jan. 2004
 */

#include "vorbispy_wrappers.h"
#include <string.h>
#include <vorbis/codec.h>
#include <numarray/libnumarray.h>
#include <fenv.h>

using namespace ogg;
using namespace boost::python;

void oggpy_wrappers_init()
{
  import_libnumarray();
}


// Notes:
// synthesis_pcmout() does not accumulate sound samples if it is not
// called each time after synthesis_blockin()
PyObject *oggpy_synthesis_pcmout(ogg::vorbis::dsp *dsp)
{
  PyArrayObject *pcmarray = NULL;
  PyObject *retval;
  float **pcm;
  float *mono;
  int samples;
  int channels;
  fenv_t env;

  feholdexcept(&env);
  samples = dsp->synthesis_pcmout(&pcm);
  
  if(samples > 0)
  {
    channels  = dsp->get_data()->vi->channels;
    pcmarray  = NA_NewArray(NULL, tFloat32, 2, channels, samples);
    for(int i=0; i<channels; i++)
    {
      mono = (float*)((char*)NA_OFFSETDATA(pcmarray)+(pcmarray->strides[0])*i);
      memcpy((void*)mono, (void*)*(pcm+i), samples*sizeof(float));
    }
    vorbis_synthesis_read(&dsp->data, samples);
    retval = NA_ReturnOutput(NULL, pcmarray);  
  } else {
    Py_INCREF(Py_None);
    retval = Py_None;
  }
  
  feupdateenv(&env);

  return retval;
}


// If *data is None, we write zero bytes.
// This is important to do because calling analysis_wrote with zero seems to be how libvorbis knows it's the end of the stream.
int oggpy_analysis_write(ogg::vorbis::dsp *d, PyObject *data)
{
  PyArrayObject* input;
  float **buffer;
  float *channel;
  int channels;
  int len;
  int wrote;

  len = 0;
  
  if(data != Py_None)
  {
    channels = d->get_data()->vi->channels;
    input    = NA_InputArray(data, tFloat32, C_ARRAY);
    if(input != NULL)
    {
      len      = input->dimensions[1];
      buffer   = d->analysis_buffer(len); // request buffer size in number of 32-bit-float samples.

      for(int i=0; i<channels; i++)
      {
        channel = (float*)((char*)NA_OFFSETDATA(input)+(input->strides[0]*i));
        memcpy((void*)(buffer[i]), (void*)channel, len*sizeof(float));
      }
      Py_XDECREF(input);
    }
  }

  wrote = d->analysis_wrote(len);

  return wrote; 
}


boost::python::list get_comments(ogg::vorbis::comment *vco)
{
  boost::python::list comments;
  str com;
  vorbis_comment *vc;

  vc = vco->get_data();

  for(int i=0; i < vc->comments; i++) 
  {
    comments.append(str(vc->user_comments[i], vc->comment_lengths[i]));
  }

  return comments;
}
