/* Boost.Python wrapper for libshout 2                                       */
/*                                                                           */
/* This library is free software; you can redistribute it and/or             */
/* modify it under the terms of the GNU Lesser General Public                */
/* License as published by the Free Software Foundation; either              */
/* version 2.1 of the License, or (at your option) any later version.        */
/*                                                                           */
/* This library is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         */
/* Lesser General Public License for more details.                           */
/*                                                                           */
/* You should have received a copy of the GNU Lesser General Public          */
/* License along with this library; if not, write to the Free Software       */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                           */
/* For questions regarding this program contact                              */
/* Daniel Holth <dholth@fastmail.fm>                                         */

/**
 * Control libshout through a Boost.Python wrapper.
 * 
 * @author Daniel Holth
 */

#include <boost/python.hpp>
#include <boost/python/def.hpp>
#include <boost/python/module.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/exception_translator.hpp>

#include "shoutcc.h"

using namespace boost::python;

void translate(ShoutErr const& x) {
  PyErr_SetObject(PyExc_RuntimeError, make_tuple(x.ern, x.err).ptr());
}

void shoutpy_send(Shout& s, str data) {
  int len;
  char const* c_str;

  len = extract<int>(data.attr("__len__")());
  c_str = extract<char const*>(data);

  s.send((const unsigned char*)c_str, len);
}

tuple shoutpy_shout_version() {
  int major, minor, patch;
  const char *name;

  name = shout_version(&major, &minor, &patch);

  return make_tuple(name, make_tuple(major, minor, patch));
}

BOOST_PYTHON_MODULE_INIT(shoutpy)
{
  register_exception_translator<ShoutErr>(&translate);
  
  // init is automatic in libshout-2.0.0, and shutdown (might) crash python, which is against the rules.
  // def("init", &shout_init);
  // def("shutdown", &shout_shutdown);

  scope().attr("version") = shoutpy_shout_version();

#ifdef SHOUT_FORMAT_VORBIS
  scope().attr("FORMAT_VORBIS")     = SHOUT_FORMAT_VORBIS;
#endif
#ifdef SHOUT_FORMAT_MP3
  scope().attr("FORMAT_MP3")        = SHOUT_FORMAT_MP3;
#endif
#ifdef SHOUT_FORMAT_OGG
  scope().attr("FORMAT_OGG")        = SHOUT_FORMAT_OGG;
#endif

  scope().attr("PROTOCOL_HTTP")       = SHOUT_PROTOCOL_HTTP;
  scope().attr("PROTOCOL_XAUDIOCAST") = SHOUT_PROTOCOL_XAUDIOCAST;
  scope().attr("PROTOCOL_ICY")        = SHOUT_PROTOCOL_ICY;

  scope().attr("AI_BITRATE")    = SHOUT_AI_BITRATE;
  scope().attr("AI_SAMPLERATE") = SHOUT_AI_SAMPLERATE;
  scope().attr("AI_CHANNELS")   = SHOUT_AI_CHANNELS;
  scope().attr("AI_QUALITY")    = SHOUT_AI_QUALITY;

  class_<Shout>("Shout", "Shout() -> new libshout2 connection object.")
    .add_property("version", &shoutpy_shout_version)
//                  "libshout2 version as a (string, values) tuple, eg. ('2.0', (2, 0, 0)).")
    .add_property("error", &Shout::get_error)
//                  "description of this connection's last error.")
    .add_property("errno", &Shout::get_errno)
//                  "error code of this connection's last error.")
    .add_property("connected", &Shout::get_connected)
//                  "boolean connection status.")
    .add_property("agent", &Shout::get_agent, &Shout::set_agent)
//                  "user agent header.")
    .add_property("description", &Shout::get_description, &Shout::set_description)
//                  "description of the stream.")
    .add_property("dumpfile", &Shout::get_dumpfile, &Shout::set_dumpfile)
//                  "filename for a server-side archive of the stream")
    .add_property("format", &Shout::get_format, &Shout::set_format)
//                  "audio format. shoutpy.FORMAT_VORBIS or shoutpy.FORMAT_MP3.")
    .add_property("genre", &Shout::get_genre, &Shout::set_genre)
//                  "genre. usually a list (whitespace-delimited string) of keywords.")
    .add_property("host", &Shout::get_host, &Shout::set_host)
//                  "IP address or dns hostname of server.")
    .add_property("mount", &Shout::get_mount, &Shout::set_mount)
//                  "mountpoint for this stream.")
    .add_property("name", &Shout::get_name, &Shout::set_name)
    .add_property("password", &Shout::get_password, &Shout::set_password)
    .add_property("port", &Shout::get_port, &Shout::set_port)
//                  "server port. default is 8000.")
    .add_property("protocol", &Shout::get_protocol, &Shout::set_protocol)
//                  "PROTOCOL_HTTP, PROTOCOL_XAUDIOCAST or PROTOCOL_ICY")
    .add_property("public", &Shout::get_public, &Shout::set_public)
//                  "should the server advertize this stream?")
    .add_property("url", &Shout::get_url, &Shout::set_url)
//                  "URL of a page describing this stream.")
    .add_property("user", &Shout::get_user, &Shout::set_user)
//                  "Username for authenticating with the server.")
    .def("get_audio_info", &Shout::get_audio_info,
         "x.get_audio_info(\"keyword\") -> value")
    .def("set_audio_info", &Shout::set_audio_info,
         "x.set_audio_info(\"keyword\", \"value\") -> None. Set information about the audio itself (channels, bitrate, ...).")
    .def("open", &Shout::open,
         "x.open() -> None. Connect to the server.")
    .def("close", &Shout::close,
         "x.close() -> None. Disconnect from the server.")
    .def("send", &shoutpy_send,
         "x.send(data) -> None. Send audio to the server.")
    // .def("send_raw", &Shout::send_raw) -- intentionally unexposed
    .def("sync", &Shout::sync,
         "x.sync() -> None. Block until the server is ready for more data.")
    .def("delay", &Shout::delay,
         "x.delay() -> milliseconds until server will need more audio data.");

  class_<ShoutMetadata>("ShoutMetadata")
    .def("add", &ShoutMetadata::add);
}
