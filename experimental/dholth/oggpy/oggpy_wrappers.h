/* Some wrappers for oggpy.
 *
 * Daniel Holth <dholth@fastmail.fm>, 2004
 */

#ifndef VORBISPY_WRAPPERS_H
#define VORBISPY_WRAPPERS_H

#include "oggcc.h"
#include <boost/python.hpp>

namespace ogg {
    int sync_write(ogg::sync &s, boost::python::str &data);
}

#endif
