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
        /// libtheora error codes
        enum theora_errors {
            ocFault     = OC_FAULT,
            ocEinval    = OC_EINVAL,
            ocBadHeader = OC_BADHEADER,
            ocNotFormat = OC_NOTFORMAT,
            ocImpl      = OC_IMPL,
            ocBadPacket = OC_BADPACKET,
            ocNewPacket = OC_NEWPACKET
        };
        
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
        class yuv_image : public yuv_buffer
        {
            public:
                yuv_buffer *get_data()  { return (yuv_buffer*)this; }
                
                PyObject* tostring () { 
                    int crop_offset=0;
                    int i,j;
                    char  y[this->y_width*this->y_height];
                    char  u[this->y_width*this->y_height];
                    char  v[this->y_width*this->y_height];
                    char  data[this->y_width*this->y_height*3];

                    //crop_offset=ti.offset_x+yuv.y_stride*ti.offset_y;
                    for(i=0;i<this->y_height;i++){
                        std::memcpy(y+i*this->y_width,
                                this->y+crop_offset+this->y_stride*i,this->y_width);
                    }
                    //  crop_offset=(ti.offset_x/2)+(yuv.uv_stride)*(ti.offset_y/2);
                    for(i=0;i<this->uv_height;i++) {
                        std::memcpy(u+i*this->uv_width,
                                this->v+crop_offset+this->uv_stride*i,this->uv_width);
                        std::memcpy(v+i*this->uv_width,
                                this->v+crop_offset+this->uv_stride*i,this->uv_width);
                    }

                    for(i=0;i<this->uv_height;i++) {
                        for(j=0;j<this->uv_width;j++) {
                            int uv_count=(i/2)*(j/2);
                            data[i*j*3]=y[i*j];
                            data[i*j*3+1]=u[uv_count];
                            data[i*j*3+2]=v[uv_count];
                        }
                    }
                    return PyString_FromStringAndSize(data,this->y_width*this->y_height*3); 
                }
                
                PyObject* tostrings () { 
                    int crop_offset=0;
                    int i;
                    char  y[this->y_width*this->y_height];
                    char  u[this->uv_width*this->uv_height];
                    char  v[this->uv_width*this->uv_height];

                    //crop_offset=theora_info::offset_x+this->y_stride*theora_info::offset_y;
                    for(i=0;i<this->y_height;i++) {
                        std::memcpy(y+i*this->y_width,
                                this->y+crop_offset+this->y_stride*i,this->y_width);
                    }
                    
                    //crop_offset=theora_info::offset_x/2+this->y_stride*theora_info::offset_y/2;
                    for(i=0; i < this->uv_height; i++) {
                        std::memcpy(u+i*this->uv_width, this->u+crop_offset+this->uv_stride*i,this->uv_width);
                        std::memcpy(v+i*this->uv_width, this->v+crop_offset+this->uv_stride*i,this->uv_width);
                    }

                    PyObject *tuple, *string;
                    tuple = PyTuple_New(3);
                    if (tuple == NULL) {
                        return NULL;
                    }
                    
                    string = PyString_FromStringAndSize(y,this->y_width*this->y_height);
                    PyTuple_SET_ITEM(tuple, 0, string);
                    string = PyString_FromStringAndSize(u,this->uv_width*this->uv_height);
                    PyTuple_SET_ITEM(tuple, 1, string);
                    string = PyString_FromStringAndSize(v,this->uv_width*this->uv_height);
                    PyTuple_SET_ITEM(tuple, 2, string);
                    return tuple;
                }
        };
	


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
