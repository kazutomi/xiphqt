/* Extras Python needs to talk to theoracc.
 *
 * Daniel Holth <dholth@fastmail.fm>
 * 10 Mar. 2004
 */

#include <theora/theora.h>
#include "theora_wrappers.h"

using namespace ogg;
using namespace boost::python;

/* unused
boost::python::list get_comments(ogg::theora::comment *vco)
{
  boost::python::list comments;
  str com;
  theora_comment *vc;

  vc = vco->get_data();

  for(int i=0; i < vc->comments; i++) 
  {
    comments.append(str(vc->user_comments[i], vc->comment_lengths[i]));
  }

  return comments;
}
*/


PyObject* tostrings (ogg::theora::yuv_image *yuv, ogg::theora::info *theora_info)
{ 
    int crop_offset=0;
    int i;

    PyObject *ystring;
    PyObject *ustring;
    PyObject *vstring;

    ystring = PyString_FromStringAndSize(NULL, yuv->y_width*yuv->y_height);
    ustring = PyString_FromStringAndSize(NULL, yuv->uv_width*yuv->uv_height);
    vstring = PyString_FromStringAndSize(NULL, yuv->uv_width*yuv->uv_height);

    char *y;
    char *u;
    char *v;

    y = PyString_AS_STRING(ystring);
    u = PyString_AS_STRING(ustring);
    v = PyString_AS_STRING(vstring);

    crop_offset=theora_info->offset_x + yuv->y_stride * theora_info->offset_y;
    for(i=0; i < yuv->y_height; i++)
    {
        std::memcpy(y+i*yuv->y_width,
                    yuv->y+crop_offset+yuv->y_stride*i, 
                    yuv->y_width);
    }

    crop_offset=theora_info->offset_x/2 + yuv->uv_stride * theora_info->offset_y/2;
    for(i=0; i < yuv->uv_height; i++)
    {        
        std::memcpy(u+i*yuv->uv_width, 
                    yuv->u+crop_offset+yuv->uv_stride*i, 
                    yuv->uv_width);
        
        std::memcpy(v+i*yuv->uv_width, 
                    yuv->v+crop_offset+yuv->uv_stride*i, 
                    yuv->uv_width);
    }

    PyObject *tuple;    
    tuple = PyTuple_New(3);

    if (tuple != NULL) {
        PyTuple_SET_ITEM(tuple, 0, ystring);
        PyTuple_SET_ITEM(tuple, 1, ustring);
        PyTuple_SET_ITEM(tuple, 2, vstring);
    }

    return tuple;
}

