#ifndef THEORACC_H
#define THEORACC_H

#include "oggcc.h"
#include <string>
#include <theora/theora.h>

#if 0

typedef struct {
    int   y_width;
    int   y_height;
    int   y_stride;

    int   uv_width;
    int   uv_height;
    int   uv_stride;
    char *y;
    char *u;
    char *v;

} yuv_buffer;

typedef enum {
  OC_CS_UNSPECIFIED,
  OC_CS_ITU_REC_470M,
  OC_CS_ITU_REC_470BG,
} theora_colorspace;

typedef struct {
  ogg_uint32_t  width;
  ogg_uint32_t  height;
  ogg_uint32_t  frame_width;
  ogg_uint32_t  frame_height;
  ogg_uint32_t  offset_x;
  ogg_uint32_t  offset_y;
  ogg_uint32_t  fps_numerator;
  ogg_uint32_t  fps_denominator;
  ogg_uint32_t  aspect_numerator;
  ogg_uint32_t  aspect_denominator;
  theora_colorspace colorspace;
  int           target_bitrate;
  int           quality;
  int           quick_p;  /* quick encode/decode */

  /* decode only */
  unsigned char version_major;
  unsigned char version_minor;
  unsigned char version_subminor;

  void *codec_setup;

  /* encode only */
  int           dropframes_p;
  int           keyframe_auto_p;
  ogg_uint32_t  keyframe_frequency;
  ogg_uint32_t  keyframe_frequency_force;  /* also used for decode init to
                                              get granpos shift correct */
  ogg_uint32_t  keyframe_data_target_bitrate;
  ogg_int32_t   keyframe_auto_threshold;
  ogg_uint32_t  keyframe_mindistance;
  ogg_int32_t   noise_sensitivity;
  ogg_int32_t   sharpness;

} theora_info;

#define OC_FAULT       -1
#define OC_EINVAL      -10
#define OC_BADHEADER   -20
#define OC_NOTFORMAT   -21
#define OC_VERSION     -22
#define OC_IMPL        -23
#define OC_BADPACKET   -24
#define OC_NEWPACKET   -25

#endif

namespace ogg {
    /// libtheora
    namespace theora {
        /// Annotate theora streams.
        class comment {
            private:
                theora_comment data;

                void init ( )   { theora_comment_init ( this->get_data() ); }
                void clear ( )  { theora_comment_clear ( this->get_data() ); }
                
            public:
                comment()   { this->init(); }
                ~comment()  { this->clear(); }
                
                theora_comment *get_data() { return &this->data; }
                
                const char *query (char *tag , int count) { 
                    return theora_comment_query (this->get_data(), tag , count ); }

                /// Return the nth comment.
                std::string query_index(unsigned int index) { 
                    theora_comment *data = this->get_data();
                    return (index < (unsigned)(data->comments) ? 
                            std::string(data->user_comments[index], data->comment_lengths[index]) : ""); };
                    
                int query_count (char *tag) { return  theora_comment_query_count ( this->get_data(), tag ); }
                int encode (packet *op)     { return  theora_encode_comment ( this->get_data(), op->get_data() ); }

                void add ( char *comment )  { return  theora_comment_add ( this->get_data(), comment ); }
                void add_tag ( char *tag , char *value )  { return  theora_comment_add_tag ( this->get_data(), tag, value ); }

                const char* get_vendor() { return this->get_data()->vendor; }

                int size()  { return this->get_data()->comments; }
        };

        
        /// Store stream information such as resolution, framerate, and aspect ratio.
        class info : public theora_info {
            private:
                void init ( )   { return  theora_info_init ( this->get_data() ); }
                void clear ( )  { return  theora_info_clear ( this->get_data() ); }

            public:
                info()  { this->init(); }
                ~info() { this->clear(); }

                theora_info *get_data() { return (theora_info*)this; }

                int decode_header ( theora::comment *cc , ogg::packet *op )  { 
                    return  theora_decode_header ( this->get_data() , cc->get_data() , op->get_data() ); }
        };
        

        /// YUV image data
        class yuv_image : public yuv_buffer { };

        
        /// Codec state.
        class state {
            private:
                theora_state data;
                void clear ( )  { return  theora_clear ( this->get_data() ); }

            public:
                state() { 
                    theora_state *ts = this->get_data(); 
                    ts->i = NULL;
                    ts->internal_encode = ts->internal_decode = NULL; 
                    ts->granulepos = 0; }

                ~state() { this->clear(); }
                
                theora_state *get_data() { return &this->data; }
                
                double granule_time ( ogg_int64_t granulepos ) {
                    return  theora_granule_time ( this->get_data(), granulepos ); }
                
                int decode_init     ( info *c )         { return theora_decode_init     ( this->get_data(), c->get_data() ); }
                int decode_packetin ( ogg::packet *op ) { return theora_decode_packetin ( this->get_data(), op->get_data() ); }
                int decode_YUVout   ( yuv_image *yuv )  { return theora_decode_YUVout   ( this->get_data(), yuv ); }
                
                int encode_init     ( info *c )         { return theora_encode_init     ( this->get_data(), c->get_data() ); }
                int encode_header   ( ogg::packet *op ) { return theora_encode_header   ( this->get_data(), op->get_data() ); }
                int encode_tables   ( ogg::packet *op ) { return theora_encode_tables   ( this->get_data(), op->get_data() ); }
                int encode_YUVin    ( yuv_image *yuv )  { return theora_encode_YUVin    ( this->get_data(), yuv ); }
                
                int encode_packetout ( int last_p, ogg::packet *op )  { 
                    return  theora_encode_packetout ( this->get_data(), last_p, op->get_data() ); }
        };


        /// Human-readable version of libtheora
        const char *version_string  ( void )    { return  theora_version_string ( ); }

        /// Computer-convenient numeric version of libtheora
        ogg_uint32_t version_number ( void )    { return  theora_version_number ( ); }
    };
};

#endif
