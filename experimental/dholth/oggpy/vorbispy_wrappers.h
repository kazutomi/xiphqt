/* vorbiscc wrappers for vorbispy.
 *
 * Daniel Holth <dholth@fastmail.fm>, 2004
 */

#ifndef VORBISPY_WRAPPERS_H
#define VORBISPY_WRAPPERS_H

#include "oggcc.h"
#include "vorbiscc.h"
#include <boost/python.hpp>

int oggpy_analysis_write(ogg::vorbis::dsp *d, PyObject *data);
boost::python::list get_comments(ogg::vorbis::comment *vc);
PyObject *oggpy_synthesis_pcmout(ogg::vorbis::dsp *dsp);

void oggpy_wrappers_init();

#endif
