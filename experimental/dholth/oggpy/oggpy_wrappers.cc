/* Wrappers for oggpy.
 *
 * Daniel Holth <dholth@fastmail.fm>, 2004
 */

#include "oggpy_wrappers.h"
#include <string.h>

using namespace ogg;
using namespace boost::python;

// should return zero.  -1 means we overflowed a buffer.
int ogg::sync_write(ogg::sync &s, str &data)
{
  int len;
  char const* c_str;
  char *buffer;

  len = extract<int>(data.attr("__len__")());
  c_str = extract<char const*>(data);

  buffer = s.buffer(len);
  memcpy((void*)buffer, (void*)c_str, len);

  return s.wrote(len);
}

