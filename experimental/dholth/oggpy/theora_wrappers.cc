/* Extras Python needs to talk to theoracc.
 *
 * Daniel Holth <dholth@fastmail.fm>
 * 10 Mar. 2004
 */

#include "theora_wrappers.h"
#include <theora/theora.h>
#include <string.h>
#include <boost/version.hpp>

using namespace ogg;
using namespace boost::python;


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
