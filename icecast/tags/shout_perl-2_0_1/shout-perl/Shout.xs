#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <shout/shout.h>

static int
not_here(char *s)
{
   croak("%s not implemented on this architecture", s);
   return -1;
}

static const char *
strconstant(const char *name, int arg)
{
  errno = 0;

  if (strEQ(name,"SHOUT_AI_BITRATE"))
    return SHOUT_AI_BITRATE;

  if (strEQ(name,"SHOUT_AI_SAMPLERATE"))
    return SHOUT_AI_SAMPLERATE;

  if (strEQ(name,"SHOUT_AI_CHANNELS"))
    return SHOUT_AI_CHANNELS;

  if (strEQ(name,"SHOUT_AI_QUALITY"))
    return SHOUT_AI_QUALITY;

  errno = EINVAL;  
  return NULL;
}
	
static double
constant(char *name, int arg)
{
   errno = 0;

   if (strEQ(name,"SHOUTERR_SUCCESS"))
     return SHOUTERR_SUCCESS;
   if (strEQ(name,"SHOUTERR_INSANE"))
     return SHOUTERR_INSANE;
   if (strEQ(name,"SHOUTERR_NOCONNECT"))
     return SHOUTERR_NOCONNECT;
   if (strEQ(name,"SHOUTERR_NOLOGIN"))
     return SHOUTERR_NOLOGIN;
   if (strEQ(name,"SHOUTERR_SOCKET"))
     return SHOUTERR_SOCKET;
   if (strEQ(name,"SHOUTERR_MALLOC"))
     return SHOUTERR_MALLOC;
   if (strEQ(name,"SHOUTERR_METADATA"))
     return SHOUTERR_METADATA;
   if (strEQ(name,"SHOUTERR_CONNECTED"))
     return SHOUTERR_CONNECTED;
   if (strEQ(name,"SHOUTERR_UNCONNECTED"))
     return SHOUTERR_UNCONNECTED;
   if (strEQ(name,"SHOUTERR_UNSUPPORTED"))
     return SHOUTERR_UNSUPPORTED;
   if (strEQ(name, "SHOUT_FORMAT_MP3"))
     return SHOUT_FORMAT_MP3;
   if (strEQ(name, "SHOUT_FORMAT_VORBIS"))
     return SHOUT_FORMAT_VORBIS;
   if (strEQ(name, "SHOUT_PROTOCOL_ICY"))
     return SHOUT_PROTOCOL_ICY;
   if (strEQ(name, "SHOUT_PROTOCOL_XAUDIOCAST"))
     return SHOUT_PROTOCOL_XAUDIOCAST;
   if (strEQ(name, "SHOUT_PROTOCOL_HTTP"))
     return SHOUT_PROTOCOL_HTTP;

   errno = EINVAL;
   return 0;
}


MODULE = Shout		PACKAGE = Shout		

PROTOTYPES: ENABLE

const char *
strconstant(name, arg)
  const char * name
  int          arg

double
constant(name,arg)
	char *		name
	int		arg

void
shout_init()

void
shout_shutdown()

shout_t *
raw_new(CLASS)
	char *CLASS
	CODE:
	RETVAL=(shout_t *)shout_new();   /* typecast so perl won't try to cvt */
	if (RETVAL == NULL) {
		warn("unable to allocate shout_t");
		XSRETURN_UNDEF;
	}
	OUTPUT:
	RETVAL

void
DESTROY(self)
	shout_t *self
	CODE:
	shout_free(self);

void
shout_set_host(self, str)
	shout_t *self
	const char *str

void
shout_set_port(self, num)
	shout_t *self
	int num

void
shout_set_mount(self, str)
	shout_t *self
	char *str

void
shout_set_password(self, str)
	shout_t *self
	char *str

void
shout_set_dumpfile(self, str)
	shout_t *self
	char *str

void
shout_set_name(self, str)
	shout_t *self
	char *str

void
shout_set_url(self, str)
	shout_t *self
	char *str

void
shout_set_genre(self, str)
	shout_t *self
	char *str

void
shout_set_description(self, str)
	shout_t *self
	char *str

void
shout_set_public(self, num)
	shout_t *self
	int num

const char *
shout_get_host(self)
	shout_t *self

unsigned short
shout_get_port(self)
	shout_t *self

const char *
shout_get_mount(self)
	shout_t *self

const char *
shout_get_password(self)
	shout_t *self

const char *
shout_get_dumpfile(self)
	shout_t *self

const char *
shout_get_name(self)
	shout_t *self

const char *
shout_get_url(self)
	shout_t *self

const char *
shout_get_genre(self)
	shout_t *self

const char *
shout_get_description(self)
	shout_t *self

int
shout_get_public(self)
	shout_t *self

const char *
shout_get_error(self)
	shout_t *self

int
shout_get_errno(self)
	shout_t *self

int
shout_get_format(self)
	shout_t *self

void
shout_set_format(self,format)
	shout_t *self
	int format

int
shout_get_protocol(self)
	shout_t *self

void
shout_set_protocol(self,protocol)
	shout_t *self
	int protocol

void *
shout_new()

void
shout_free(self)
	shout_t *self

int
shout_open(self)
	shout_t *self

int
shout_close(self)
	shout_t *self

int
shout_send(self, buff, len)
       shout_t *self
       unsigned char *buff
       unsigned long len

void
shout_sync(self)
       shout_t *self

int
shout_delay(self)
       shout_t *self

int
shout_set_audio_info(self, name, value)
       shout_t *self
       const char *name
       const char *value

const char *
shout_get_audio_info(self, name)
       shout_t *self
       const char *name

shout_metadata_t *
shout_metadata_new()

void 
shout_metadata_free(md)
	shout_metadata_t *md

int 
shout_metadata_add(md,name,value)
	shout_metadata_t *md
	 char *name
	 char *value

int 
shout_set_metadata(self,md)
	shout_t *self
	shout_metadata_t *md

