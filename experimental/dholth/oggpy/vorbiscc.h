/** vorbiscc.h
 *
 * A straightforward C++ wrapper for libvorbis.
 *
 * @author Daniel Holth <dholth@fastmail.fm>
 */

#ifndef VORBISCC_H
#define VORBISCC_H

#include "oggcc.h"
#include <string>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

namespace ogg {
    /// libvorbis
    namespace vorbis {
        /// interestingly, the vorbis spec in theory supports 8-bit clean comments, but the comment
        /// manipulators only accept null-terminated strings.  however, if you did somehow put a null 
        /// in the middle of a vorbis comment you risk being compatible with only yourself.
        class comment {
            protected:
                void init ( )   { vorbis_comment_init ( &this->data ); }
                void clear ( )  { vorbis_comment_clear ( &this->data ); }

                int header_out ( ogg_packet *op )  {
                    return vorbis_commentheader_out ( &this->data, op ); }

            public:
                vorbis_comment data;
                vorbis_comment *get_data()  { return &this->data; }

                comment()   { this->init(); }
                ~comment()  { this->clear(); }

                const char *query ( char *tag , int count )  {
                    return vorbis_comment_query ( &this->data, tag, count ); }

                int query_count ( char *tag )  {
                    return vorbis_comment_query_count ( &this->data, tag ); }

                void add ( char *comment )  {
                    vorbis_comment_add ( &this->data, comment ); }

                void add_tag ( char *tag , char *contents )  {
                    vorbis_comment_add_tag ( &this->data, tag, contents ); }

                int header_out ( packet *op )  {
                    return this->header_out ( op->get_data() ); }

                /// Return the nth comment.
                std::string query_index(unsigned int index) { 
                    vorbis_comment *data = this->get_data();
                    return (index < (unsigned)(data->comments) ? 
                            std::string(data->user_comments[index], data->comment_lengths[index]) : ""); };
                
                int size()  { return this->get_data()->comments; }
        };


        /// Center of vorbis decoding.  Stores lots of bitstream metadata.
        class info {
            protected:
                int synthesis_headerin ( vorbis_comment *vc, ogg_packet *op )  {
                    return vorbis_synthesis_headerin ( &this->data, vc, op );  }

                long packet_blocksize ( ogg_packet *op )  {
                    return vorbis_packet_blocksize ( &this->data, op );  }

                void info_init ( )  { return vorbis_info_init ( &this->data );  }
                void info_clear ( )  { return vorbis_info_clear ( &this->data );  }

            public:
                vorbis_info data;
                vorbis_info *get_data() { return &this->data; }

                info()    { this->info_init(); }
                ~info()   { this->info_clear(); }

                int encode_init(long channels,
                                long rate, 
                                long max_bitrate, 
                                long nominal_bitrate, 
                                long min_bitrate)

                { return vorbis_encode_init(&this->data,
                                            channels, 
                                            rate, 
                                            max_bitrate, 
                                            nominal_bitrate, 
                                            min_bitrate); } 


                int encode_setup_managed(long channels, 
                                         long rate, 
                                         long max_bitrate, 
                                         long nominal_bitrate, 
                                         long min_bitrate) { 

                    return vorbis_encode_setup_managed(&this->data, 
                                                       channels, 
                                                       rate, 
                                                       max_bitrate,
                                                       nominal_bitrate, 
                                                       min_bitrate); }

                /// quality from 0. (lo) to 1. (hi)
                int encode_setup_vbr(long channels, 
                                     long rate, 
                                     float base_quality) {

                    return vorbis_encode_setup_vbr(&this->data, 
                                                   channels, 
                                                   rate,           
                                                   base_quality); }


                int encode_init_vbr(long channels, 
                                    long rate, 
                                    float base_quality) {

                    return vorbis_encode_init_vbr(&this->data, 
                                                  channels, 
                                                  rate, 
                                                  base_quality); }


                int encode_ctl(vorbis_info *vi, int number, void *arg) {
                    return vorbis_encode_ctl( &this->data, number, arg ); }


                int encode_setup_init() {
                    return vorbis_encode_setup_init( &this->data ); }


                int info_blocksize ( int zo )  {
                    return vorbis_info_blocksize ( &this->data, zo );  }

                int synthesis_halfrate ( int flag )  {
                    return vorbis_synthesis_halfrate ( &this->data, flag );  }

                int synthesis_halfrate_p ( )  {
                    return vorbis_synthesis_halfrate_p ( &this->data );  }

                // packet in, comment out.
                int synthesis_headerin ( comment *vc, packet *op ) {
                    return this->synthesis_headerin ( &vc->data, op->get_data() ); }

                long packet_blocksize ( packet *op )  {
                    return this->packet_blocksize ( op->get_data() );  }

                int  get_version()  { return this->get_data()->version; }
                int  get_channels() { return this->get_data()->channels; }
                long get_rate()     { return this->get_data()->rate; }
        };


        /// Local state for a chunk of vorbis.  Multiple block decodes may proceed in parallel.
        class block {
            protected:
                int analysis ( ogg_packet *op )  {
                    return vorbis_analysis ( &this->data, op );  }

                int synthesis ( ogg_packet *op )  {
                    return vorbis_synthesis ( &this->data, op ); }

                int synthesis_trackonly ( ogg_packet *op )  {
                    return vorbis_synthesis_trackonly ( &this->data, op ); }

            public:
                vorbis_block data;

                int analysis ( packet *op )   { return this->analysis(op->get_data()); }
                int bitrate_addblock ( )      { return vorbis_bitrate_addblock ( &this->data ); }
                int block_clear ( )           { return vorbis_block_clear ( &this->data ); }
                int synthesis ( packet *op )  { return this->synthesis(op->get_data()); }
                int synthesis_trackonly ( packet *op )  { return this->synthesis_trackonly(op->get_data()); }
        };


        /// PCM processing.
        // may separate into separate synthesis and analysis objects.
        class dsp {
            protected:
                int bitrate_flushpacket ( ogg_packet *op )  {
                    return vorbis_bitrate_flushpacket ( &this->data, op ); }

                int analysis_init ( vorbis_info *vi )  {
                    return vorbis_analysis_init ( &this->data, vi ); }

                int analysis_blockout ( vorbis_block *vb )  {
                    return vorbis_analysis_blockout ( &this->data, vb ); }

                int analysis_headerout ( vorbis_comment *vc, ogg_packet *op, ogg_packet *op_comm, ogg_packet *op_code )  {
                    return vorbis_analysis_headerout ( &this->data, vc , op , op_comm , op_code ); }

                int synthesis_init ( vorbis_info *vi )  {
                    return vorbis_synthesis_init ( &this->data, vi ); }

                int synthesis_blockin ( vorbis_block *vb )  {
                    return vorbis_synthesis_blockin ( &this->data, vb ); }

                int block_init ( vorbis_block *vb )  {
                    return vorbis_block_init ( &this->data, vb ); }

            public:
                /// this struct holds a reference to vorbis_info
                vorbis_dsp_state data;
                vorbis_dsp_state *get_data()  { return &this->data; }

                int block_init ( block *vb )  {
                    return this->block_init ( &vb->data ); }

                double granule_time ( ogg_int64_t granulepos )  {
                    return vorbis_granule_time ( &this->data, granulepos ); }

                int bitrate_flushpacket ( packet *op )  {
                    return this->bitrate_flushpacket ( op->get_data() ); }

                ////////////// ANALYSIS //////////////////

                int analysis_init ( info *vi )  {
                    return this->analysis_init ( &vi->data ); }

                float **analysis_buffer ( int vals )  {
                    return vorbis_analysis_buffer ( &this->data, vals ); }

                int analysis_wrote ( int vals )  {
                    return vorbis_analysis_wrote ( &this->data, vals ); }

                int analysis_blockout ( block *vb )  {
                    return this->analysis_blockout ( &vb->data ); }        

                // spits out three headers.
                int analysis_headerout ( comment *vc, 
                                         packet *op, 
                                         packet *op_comm, 
                                         packet *op_code )  {
                    return this->analysis_headerout ( &vc->data , 
                                                      op->get_data() , 
                                                      op_comm->get_data() , 
                                                      op_code->get_data() ); }


                ////////////// SYNTHESIS //////////////////

                int synthesis_init ( info *vi )  {
                    return this->synthesis_init ( &vi->data ); }

                int synthesis_blockin ( block *vb )  {
                    return this->synthesis_blockin ( &vb->data ); }

                int synthesis_lapout ( float ***pcm )  {
                    return vorbis_synthesis_lapout ( &this->data, pcm ); }

                int synthesis_pcmout ( float ***pcm )  {
                    return vorbis_synthesis_pcmout ( &this->data, pcm ); }

                int synthesis_read ( int samples )  {
                    return vorbis_synthesis_read ( &this->data, samples ); }

                int synthesis_restart ( )  {
                    return vorbis_synthesis_restart ( &this->data ); }


                void dsp_clear ( )  {
                    vorbis_dsp_clear ( &this->data ); }
        };
    };
};

#endif

/* Vorbis ERRORS and return codes **

#define OV_FALSE      -1  
#define OV_EOF        -2
#define OV_HOLE       -3

#define OV_EREAD      -128
#define OV_EFAULT     -129
#define OV_EIMPL      -130
#define OV_EINVAL     -131
#define OV_ENOTVORBIS -132
#define OV_EBADHEADER -133
#define OV_EVERSION   -134
#define OV_ENOTAUDIO  -135
#define OV_EBADPACKET -136
#define OV_EBADLINK   -137
#define OV_ENOSEEK    -138

 *************************************/
