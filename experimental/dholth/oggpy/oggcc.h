/* oggcc.h
 *
 * A straightforward C++ wrapper for libogg.
 *
 * Daniel Holth <dholth@fastmail.fm>
 * 19 Jan. 2004
 */

#ifndef OGGCC_H
#define OGGCC_H

#include <ogg/ogg.h>
#include <vorbis/codec.h>

namespace ogg {

  class packet {
    public:
      ogg_packet data;
      
      void clear ()               { ogg_packet_clear(&this->data); }
  };

  class page {
    public:
      ogg_page data;
      
      int bos ( )                 { return ogg_page_bos(&this->data); }
      int continued ( )           { return ogg_page_continued(&this->data); }
      int eos ( )                 { return ogg_page_eos(&this->data); }
      int packets ( )             { return ogg_page_packets(&this->data); }
      int serialno ( )            { return ogg_page_serialno(&this->data); }
      int version ( )             { return ogg_page_version(&this->data); }
      long pageno ( )             { return ogg_page_pageno(&this->data); }
      ogg_int64_t granulepos ( )  { return ogg_page_granulepos(&this->data); }
      void checksum_set ( )       { return ogg_page_checksum_set(&this->data); }
  };

  class stream {
    public:
      ogg_stream_state data;
      
      int clear ( )                       { return ogg_stream_clear(&this->data); }
      int destroy ( )	                    { return ogg_stream_destroy(&this->data); }
      int eos ( )	                        { return ogg_stream_eos(&this->data); }
      int flush ( ogg_page *og )  	      { return ogg_stream_flush(&this->data, og); }
      int init ( int serialno )	          { return ogg_stream_init(&this->data, serialno); }
      int packetin ( ogg_packet *op )	    { return ogg_stream_packetin(&this->data, op); }
      int packetout ( ogg_packet *op )	  { return ogg_stream_packetout(&this->data, op); }
      int packetpeek ( ogg_packet *op )	  { return ogg_stream_packetpeek(&this->data, op); }
      int pagein  ( ogg_page *og )	      { return ogg_stream_pagein(&this->data, og); }
      int pageout ( ogg_page *og )	      { return ogg_stream_pageout(&this->data, og); }
      int reset   ( )	                    { return ogg_stream_reset(&this->data); }
      int reset_serialno ( int serialno )	{ return ogg_stream_reset_serialno(&this->data, serialno); }
  };

  class sync {
    public:
      ogg_sync_state data;
      
      char *buffer    ( long size )	      { return ogg_sync_buffer (&this->data, size); }
      int   clear     ( )	                { return ogg_sync_clear (&this->data); }
      int   destroy   ( )	                { return ogg_sync_destroy (&this->data); }
      int   init      ( )             	  { return ogg_sync_init (&this->data); }
      int   pageout   ( ogg_page *og )	  { return ogg_sync_pageout (&this->data, og); }
      int   reset     ( )	                { return ogg_sync_reset (&this->data); }
      int   wrote     ( long bytes )	    { return ogg_sync_wrote (&this->data, bytes); }
      long  pageseek  ( ogg_page *og )	  { return ogg_sync_pageseek (&this->data, og); }
  };

  
  namespace vorbis {

    class block {
      public:
        vorbis_block data;

        int analysis ( ogg_packet *op )  {
          return vorbis_analysis ( &this->data, op );  }
        
        int bitrate_addblock ( )  {
          return vorbis_bitrate_addblock ( &this->data ); }
        
        int block_clear ( )  {
          return vorbis_block_clear ( &this->data ); }
        
        int synthesis ( ogg_packet *op )  {
          return vorbis_synthesis ( &this->data, op ); }
        
        int synthesis_trackonly ( ogg_packet *op )  {
          return vorbis_synthesis_trackonly ( &this->data, op ); }
    };

    
    class comment {
      public:
        vorbis_comment data;

        char *query ( char *tag , int count )  {
          return vorbis_comment_query ( &this->data, tag, count ); }

        int query_count ( char *tag )  {
          return vorbis_comment_query_count ( &this->data, tag ); }

        int header_out ( ogg_packet *op )  {
          return vorbis_commentheader_out ( &this->data, op ); }

        void add ( char *comment )  {
          vorbis_comment_add ( &this->data, comment ); }

        void add_tag ( char *tag , char *contents )  {
          vorbis_comment_add_tag ( &this->data, tag, contents ); }

        void clear ( )  {
          vorbis_comment_clear ( &this->data ); }

        void init ( )  {
          vorbis_comment_init ( &this->data ); }
    };

    
    class dsp {
      public:
        vorbis_dsp_state data;

        double granule_time ( ogg_int64_t granulepos )  {
          return vorbis_granule_time ( &this->data, granulepos ); }

        float **analysis_buffer ( int vals )  {
          return vorbis_analysis_buffer ( &this->data, vals ); }

        int analysis_blockout ( vorbis_block *vb )  {
          return vorbis_analysis_blockout ( &this->data, vb ); }

        int analysis_headerout ( vorbis_comment *vc , ogg_packet *op , ogg_packet *op_comm , ogg_packet *op_code )  {
          return vorbis_analysis_headerout ( &this->data, vc , op , op_comm , op_code ); }

        int analysis_init ( vorbis_info *vi )  {
          return vorbis_analysis_init ( &this->data, vi ); }

        int analysis_wrote ( int vals )  {
          return vorbis_analysis_wrote ( &this->data, vals ); }

        int bitrate_flushpacket ( ogg_packet *op )  {
          return vorbis_bitrate_flushpacket ( &this->data, op ); }

        int block_init ( vorbis_block *vb )  {
          return vorbis_block_init ( &this->data, vb ); }

        int synthesis_blockin ( vorbis_block *vb )  {
          return vorbis_synthesis_blockin ( &this->data, vb ); }

        int synthesis_init ( vorbis_info *vi )  {
          return vorbis_synthesis_init ( &this->data, vi ); }

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


    class info {
      public:
        vorbis_info data;

        int info_blocksize ( int zo )  {
          return vorbis_info_blocksize ( &this->data, zo );  }

        int synthesis_halfrate ( int flag )  {
          return vorbis_synthesis_halfrate ( &this->data, flag );  }

        int synthesis_halfrate_p ( )  {
          return vorbis_synthesis_halfrate_p ( &this->data );  }

        int synthesis_headerin ( vorbis_comment *vc, ogg_packet *op )  {
          return vorbis_synthesis_headerin ( &this->data, vc, op );  }

        long packet_blocksize ( ogg_packet *op )  {
          return vorbis_packet_blocksize ( &this->data, op );  }

        void info_clear ( )  {
          return vorbis_info_clear ( &this->data );  }

        void info_init ( )  {
          return vorbis_info_init ( &this->data );  }

    };
  };
};

#endif
