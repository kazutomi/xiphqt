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

/** shoutcc.h
 *
 * Interface for the C++ wrapper to libshout2.
 *
 * @author Daniel Holth <dholth@fastmail.fm>
 */


#ifndef __SHOUTCC_H__
#define __SHOUTCC_H__

#include <shout/shout.h>
#include <string>


class ShoutMetadata;

/// The exception.
/** 
 * Wherever libshout2 returns error codes, shoutpy throws exceptions.
 */
class ShoutErr {
  public:
    /// libshout2 error code.  See shout.h
    int ern;
    /// libshout2 error description.
    std::string err;

    ShoutErr(int n);        
    ShoutErr(int n, std::string e) { 
      this->ern = n; 
      this->err = e; }
};

/// The main event.  Wraps the shout_t object in a C++ object.
class Shout {
  private:
    /// libshout2 data structure that we wrap.
    shout_t *self;

  public:
    shout_t *get_self() { return this->self; }

    Shout();
    ~Shout();

    void check(int code);

    const char *get_error();

    int get_errno();

    bool get_connected();

    void set_host(const char *host);
    const char *get_host();

    void set_port(unsigned short port);
    unsigned short get_port();

    void set_password(const char *password);
    const char *get_password();

    void set_mount(const char *mount);
    const char *get_mount();

    void set_name(const char *name);
    const char *get_name();

    void set_url(const char *url);
    const char *get_url();

    void set_genre(const char *genre);
    const char *get_genre();

    void set_user(const char *username);
    const char *get_user();

    void set_agent(const char *username);
    const char *get_agent();

    void set_description(const char *description);
    const char *get_description();

    void set_dumpfile(const char *dumpfile);
    const char *get_dumpfile();

    void set_audio_info(const char *name, const char *value);
    const char *get_audio_info(const char *name);

    void set_public(unsigned int make_public);
    unsigned int get_public();

    void set_format(unsigned int format);
    unsigned int get_format();

    void set_protocol(unsigned int protocol);
    unsigned int get_protocol();

    void open();
    void close();
    void send(const unsigned char *data, size_t len);
    ssize_t send_raw(const unsigned char *data, size_t len);
    void sync();
    int delay();

    void set_metadata(ShoutMetadata *metadata);
};

/// Wrapper for shout_metadata_t, a dictionary-like metadata object.
class ShoutMetadata {
  public:
    shout_metadata_t *self;
    
    ShoutMetadata()   { this->self = shout_metadata_new(); }
    ~ShoutMetadata()  { shout_metadata_free(this->self);   }
    
    void add(const char *name, const char *value) { shout_metadata_add(this->self, name, value); }
};

#endif
