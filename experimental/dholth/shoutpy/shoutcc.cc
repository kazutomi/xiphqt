/*  C++ wrapper for libshout2
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  For questions regarding this program contact
 *  Daniel Holth <dholth@fastmail.fm>
 */

/** \mainpage shoutpy's C++ API
 * \section Intro
 * 
 * These pages document the C++ API for libshout2, which is simply
 * libshout2's object-oriented C functions like shout_get_foo(shout_t * object, ...)
 * collected into one Shout:: class.  This library doesn't attempt to wrap everything in C++;
 * it doesn't pay off to wrap the non-object-oriented functions like shout_get_version().
 *
 * As they are all very similar, this set of documents
 * might also help you understand how to use libshout2 or shoutpy, or even pyshout.
 * In fact, most of the member documentation was copied from libshout2's documentation.
 * 
 * This package was developed for shoutpy, an alternative
 * Python wrapper for libshout2 that is implemented with
 * Boost.Python. (http://boost.org/libs/python/doc/index.html)
 *
 * Shoutpy's home is http://dingoskidneys.com/shoutpy/.
 *
 * libshout and icecast live at http://www.icecast.org/.
 *
 * \section License
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \author Daniel Holth < dholth at fastmail dot fm >
 */
  

#include "shoutcc.h"


/// Provide default descriptions for libshout2 errors when a library-provided text description is not available.
ShoutErr::ShoutErr(int n) {
  char *d;
  switch(n) {
    case SHOUTERR_SUCCESS:
      d = "SUCCESS";
      break;
    case SHOUTERR_INSANE:
      d = "INSANE";
      break;
    case SHOUTERR_NOCONNECT:
      d = "NOCONNECT";
      break;
    case SHOUTERR_NOLOGIN:
      d = "NOLOGIN";
      break;
    case SHOUTERR_SOCKET:
      d = "SOCKET";
      break;
    case SHOUTERR_MALLOC:
      d = "MALLOC";
      break;
    case SHOUTERR_METADATA:
      d = "METADATA";
      break;
    case SHOUTERR_CONNECTED:
      d = "CONNECTED";
      break;
    case SHOUTERR_UNCONNECTED:
      d = "UNCONNECTED";
      break;
    case SHOUTERR_UNSUPPORTED:
      d = "UNSUPPORTED";
      break;
    default:
      d = "";
  }

  this->err = d;
}



/// Constructor.
Shout::Shout() {
  this->self = shout_new();
}

/// Destructor.
Shout::~Shout() {
  shout_free(this->self);
}

/// Internal wrapper for libshout2 return codes.  If it was an error, throw the appropriate ShoutErr object.
void Shout::check(int code) {
  if(code!=0) {
    throw ShoutErr(code, this->get_error());
  }
}

/// \brief Return a statically allocated string describing 
/// the last shout error that occured in this connection.
/// Only valid until the next call affecting this connection.
const char* Shout::get_error() {
  return shout_get_error(this->self);
}

/// Return the shout error code of the last error that occured in this connection.
int Shout::get_errno() {
  return shout_get_errno(this->self);
}

/// Are we connected to a server?
bool Shout::get_connected() {
  return (shout_get_connected(this->self) == SHOUTERR_CONNECTED);
}

/// Set IP address or hostname of our server.
/**
 * The default is localhost.
 * \param host IP address in dotted decimal format, or hostname, of server.
 */
void Shout::set_host(const char *host) {
  this->check(shout_set_host(this->self, host));
}

/// Return IP address/hostname of our server.
const char *Shout::get_host() {
  return shout_get_host(this->self);
}

/// Set server port.
/**
 * The default is 8000.
 */
void Shout::set_port(unsigned short port) {
  this->check(shout_set_port(this->self, port));
}

/// Return server port.
unsigned short Shout::get_port() {
  return shout_get_port(this->self);
}

/// Set password to authenticate to the server with.
/**
 * This parameter @b must be set.  There is no default.
 */
void Shout::set_password(const char *password) {
  this->check(shout_set_password(this->self, password));
}

/// Return the password.
const char *Shout::get_password() {
  return shout_get_password(this->self);
}

/// Set the mount point for this stream.
/**
 * A mountpoint is a resource on the icecast server that represents
 * a single broadcast stream. Mountpoints are named similar to
 * files (/mystream.ogg, /mymp3stream). When listeners connect
 * to icecast2, they must specify the mountpoint in the request
 * (i.e. http://192.168.1.10:8000/mystream.ogg). Additionally, source
 * clients must specify a mountpoint when they connect as well. Statistics
 * are kept track of by mountpoint. Mountpoints are a fundamental aspect
 * of icecast2 and how it is organized.
 * 
 * Only for protocols that support this option. ( \c SHOUT_PROTOCOL_ICY doesn't).
 */
void Shout::set_mount(const char *mount) {
  this->check(shout_set_mount(this->self, mount));
}

/// Return this stream's mount point.
const char *Shout::get_mount() {
  return shout_get_mount(this->self);
}

/// Set the name of this stream.
/** 
 * This must be for the benefit of listing the stream, 
 * e.g. "The Emo Hamster Rock Channel" 
 */
void Shout::set_name(const char *name) {
  this->check(shout_set_name(this->self, name));
}

/// Return the name of this stream.
const char *Shout::get_name() {
  return shout_get_name(this->self);
}

/// Set the URL of a site about this stream.
void Shout::set_url(const char *url) {
  this->check(shout_set_url(this->self, url));
}

/// Return URL of the site about this stream.
const char *Shout::get_url() {
  return shout_get_url(this->self);
}

/// Set the genre of this stream.
/**
 * This is usually a keyword list, eg "techno electronica world hippity-hop".
 */ 
void Shout::set_genre(const char *genre) {
  this->check(shout_set_genre(this->self, genre));
}

/// Return the genre of this stream.
const char *Shout::get_genre() {
  return shout_get_genre(this->self);
}

/// Set username for authenticating with the server.
void Shout::set_user(const char *username) {
  this->check(shout_set_user(this->self, username));
}

/// Return the user name.
const char *Shout::get_user() {
  return shout_get_user(this->self);
}

/// Set the user agent header.
/** 
 * This is \c libshout/VERSION by default. If you don't 
 * know what this function is for, don't use it.
 */
void Shout::set_agent(const char *username) {
  this->check(shout_set_agent(this->self, username));
}

/// Return the user agent header.
const char *Shout::get_agent() {
  return shout_get_agent(this->self);
}

/// Set the description of this stream.
void Shout::set_description(const char *description) {
  this->check(shout_set_description(this->self, description));
}
  
/// Return the description of this stream.
const char *Shout::get_description() {
  return shout_get_description(this->self);
}

/// Set server-side archive filename.
/** 
 * If the server supports it, you can request that your stream be archived
 * on the server under the name \c dumpfile. This can quickly eat a lot of 
 * disk space, so think twice before setting it.
 */
void Shout::set_dumpfile(const char *dumpfile) {
  this->check(shout_set_dumpfile(this->self, dumpfile));
}

/// Return server-side archive filename.
const char *Shout::get_dumpfile() {
  return shout_get_dumpfile(this->self);
}

/// Set a stream audio parameter.
/**
 * The currently defined parameters are listed in shout.h
 * ("bitrate", "samplerate", "channels", "quality") but you 
 * are free to add additional fields if your directory server 
 * understands them.
 */
void Shout::set_audio_info(const char *name, const char *value) {
  this->check(shout_set_audio_info(this->self, name, value));
}

/// Return a stream audio parameter.
const char *Shout::get_audio_info(const char *name) {
  return shout_get_audio_info(this->self, name);
}

/// Instruct the server to advertise (or not) this stream.
/**
 * Setting this to \c 1 asks the server to list the stream in
 * any directories it knows about. To suppress listing, set this to
 * \c 0. The default is \c 0.
 */
void Shout::set_public(unsigned int make_public) {
  this->check(shout_set_public(this->self, make_public));
}

/// Return whether this stream will be public.
unsigned int Shout::get_public() {
  return shout_get_public(this->self);
}

/// Set the audio format of this stream.
/** 
 * The default is \c SHOUT_FORMAT_VORBIS
 */
void Shout::set_format(unsigned int format) {
  this->check(shout_set_format(this->self, format));
}

/// Return the audio format of the stream.
unsigned int Shout::get_format() {
  return shout_get_format(this->self);
}

/// Set the protocol with which to connect to the server.
/**
 * The default is \c SHOUT_PROTOCOL_HTTP (compatible with Icecast 2).
 */
void Shout::set_protocol(unsigned int protocol) {
  this->check(shout_set_protocol(this->self, protocol));
}

/// Return the protocol this stream will use.
unsigned int Shout::get_protocol() {
  return shout_get_protocol(this->self);
}

/// Open a connection to a server. All connection parameters must have been set prior to this call.
void Shout::open() { 
  this->check(shout_open(this->self));
}

/// Close this connection to the server.
void Shout::close() {
  this->check(shout_close(this->self));
}

/// Send audio data to the server.
/** 
 * Send \c len bytes of audio data from the buffer pointed to by
 * \c data to the server. The connection must already have been
 * established by a successful call to Shout::shout_open.
 */
void Shout::send(const unsigned char *data, size_t len) {
  this->check(shout_send(this->self, data, len));
}

/// Send data to the server, in an unfriendly, unsafe manner.
/** 
 * Send \c len bytes of audio data from the buffer pointed to by
 * \c data to the server. The data is not parsed for timing
 * or validity, but sent raw over the connection. The connection 
 * must already have been established by a successful call to
 * Shout::shout_open.
 *
 * \warning This function should not be used unless you know exactly what you
 * are doing. Its use is deprecated and it may be removed in a future version of
 * the library. 
 */
ssize_t Shout::send_raw(const unsigned char *data, size_t len) {
  return shout_send_raw(this->self, data, len);
}


/// Sleep until the server will be ready for more data.
/**
 *  Causes the caller to sleep for the amount of time necessary to play back
 *  audio sent since the last call to Shout::shout_sync.  Should be called
 *  before every call to Shout::shout_send to ensure that audio data is
 *  sent to the server at the correct speed.  Alternatively, the caller may
 *  use Shout::shout_delay to determine the number of milliseconds to wait
 *  and delay itself.
 */
void Shout::sync() {
  shout_sync(this->self);
}

/// Return delay, in milliseconds, after which server will be ready for more data.
int Shout::delay() {
  return shout_delay(this->self);
}

/// Valid only for Mp3, define the metadata (title, artist, etc.) for the current stream.
/** 
 * Currently the only metadata supported is song title and only for MP3 streams.
 *
 * Ogg/Vorbis metadata should be embedded in the Ogg stream.
 */
void Shout::set_metadata(ShoutMetadata *metadata) {
  this->check(shout_set_metadata(this->self, metadata->self));
}
