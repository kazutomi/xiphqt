/* Functions needed for Python to interface with theoracc
 *
 * Daniel Holth <dholth@fastmail.fm>, 2004
 */

#ifndef VORBISPY_WRAPPERS_H
#define VORBISPY_WRAPPERS_H

#include "oggcc.h"
#include "theoracc.h"
#include <boost/python.hpp>

/// Return a list of all the comments in a theora comment object.
///
/// Less important now that we have query_index.
boost::python::str get_comment(ogg::theora::comment *tco);

#endif
