/* Functions needed specifically for
 * Python's interface with theora/theoracc
 * (wrappers that are useful from C++
 * go in theoracc.cc)
 *  
 * Daniel Holth <dholth@fastmail.fm>, 2004
 */

#ifndef THEORAPY_WRAPPERS_H
#define THEORAPY_WRAPPERS_H

#include "oggcc.h"
#include "theoracc.h"
#include <boost/python.hpp>

/// Return a list of all the comments in a theora comment object.
///
/// Less important now that we have query_index.
// boost::python::str get_comment(ogg::theora::comment *tco);
// "Wrapper" (w)tostrings until tested.
PyObject* wtostrings(ogg::theora::yuv_image *yuv);

#endif
