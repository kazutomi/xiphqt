/** oggcc.h
 *
 * A straightforward C++ wrapper for libogg.
 *
 * @author Daniel Holth <dholth@fastmail.fm>
 */

// #include "oggpy_wrappers.h"

#ifndef OGGCC_H
#define OGGCC_H

#include <ogg/ogg.h>
#include <string>


/// enclosure for all oggcc memberse
namespace ogg {

    /// Codec-level packet.
    class packet {
        /// packet points to internal data structures.  not sure how memory management works.
        /// zero packet and free internal storage (NOT SAFE: packet points to internal oggvorbis stuff)
        /// ogg_packet_clear is not called in the ogg, vorbis, or ices source.
        protected:
            void clear ( )              { ogg_packet_clear(&this->data); }

        private:
            ogg_packet data;

        public:
            ogg_packet *get_data( )     { return &this->data; }

            packet ( )                  { /* memset(&data, 0, sizeof(this->data)); // may be redundant anyway */ }
            virtual ~packet ( )         { }

            ogg_int64_t packetno( )     { return this->get_data()->packetno; }
            ogg_int64_t granulepos( )   { return this->get_data()->granulepos; } // may want to expose only page granulepos, or this may be useful for encode only.
    };


    /// For submitting user data to an ogg stream
    class userpacket : public packet {
        private:
            std::string bytes;

        public:
            virtual ~userpacket ()    { }

            /// granulepos is the last sample (+1?) in this packet. 
            // libogg framing sets the packetno for us.
            userpacket(std::string bytes, ogg_int64_t granulepos, bool bos = false, bool eos = false)
            {
                this->bytes = bytes;
                this->get_data()->packet = (unsigned char*)bytes.c_str();
                this->get_data()->granulepos = granulepos;
                this->get_data()->bytes      = bytes.size();
                this->get_data()->b_o_s      = bos;
                this->get_data()->e_o_s      = eos;
            }
    };


    /// Shared pieces of the bitpacker.
    class oggpack_base {
        private:
            oggpack_buffer data;

        public:
            oggpack_buffer *get_data()          { return &this->data; }
            unsigned char *get_buffer ( )       { return oggpack_get_buffer ( this->get_data() ); }

            void readinit ( unsigned char *buf, int bytes )  { oggpack_readinit ( this->get_data(), buf, bytes );  }

            /** Start reading data from an ogg packet.
             *
             * libogg1 holds a reference to the packet; be careful not to
             * read after you've invalidated the packet.
             */
            void readinit ( packet *p )    {
                ogg_packet *op;
                op = p->get_data();
                this->readinit(op->packet, op->bytes);

                /* LIBOGG2:
                 * opb = malloc(oggpack_buffersize()):
                 * // oggpack_buffersize() is implemented as
                 * // sizeof(oggpack_buffer), but we don't
                 * // get that size in our main code.
                 * oggpack_readinit(op->packet);
                 */
            }
            
            void writeinit ( )                  { oggpack_writeinit ( this->get_data() );  }

            /// Probably inefficient at the moment, but it's for a Python wraper.
            ogg::packet packetout(ogg_int64_t granulepos, bool bos=false, bool eos=false) { 
                oggpack_buffer *opb = this->get_data();
                return ogg::userpacket(std::string((const char*)opb->buffer,
                                                   oggpack_bytes(opb)), granulepos, bos, eos);
            }
    };


    /// Little-endian bitpacker.
    class oggpack : public oggpack_base {
        public:
            long bits   ( )             { return oggpack_bits   ( this->get_data() );  }
            long bytes  ( )             { return oggpack_bytes  ( this->get_data() );  }
            long look   ( int bits )    { return oggpack_look   ( this->get_data(), bits );  }
            long look1  ( )             { return oggpack_look1  ( this->get_data() ); }
            long read   ( int bits )    { return oggpack_read   ( this->get_data(), bits );  }
            long read1  ( )             { return oggpack_read1  ( this->get_data() ); }

            void adv ( int bits )       { oggpack_adv   ( this->get_data(), bits );  }
            void adv1 ( )               { oggpack_adv1  ( this->get_data() );  }
            void reset ( )              { oggpack_reset ( this->get_data() );  }

            void write (unsigned long value, int bits )     { oggpack_write ( this->get_data(), value, bits ); }

            void writealign (  )                            { oggpack_writealign ( this->get_data() ); }
            void writeclear (  )                            { oggpack_writeclear ( this->get_data() ); }
            void writecopy  ( void *source, long bits )     { oggpack_writecopy ( this->get_data(), source, bits );  }
            void writetrunc ( long bits )                   { oggpack_writetrunc ( this->get_data(), bits );  }
    };

    
    /// Big-endian bitpacker
    class oggpackB : public oggpack_base {
        public:
            long bits   ( )             { return oggpackB_bits   ( this->get_data() );  }
            long bytes  ( )             { return oggpackB_bytes  ( this->get_data() );  }
            long look   ( int bits )    { return oggpackB_look   ( this->get_data(), bits );  }
            long look1  ( )             { return oggpackB_look1  ( this->get_data() ); }
            long read   ( int bits )    { return oggpackB_read   ( this->get_data(), bits );  }
            long read1  ( )             { return oggpackB_read1  ( this->get_data() ); }

            void adv ( int bits )       { oggpackB_adv   ( this->get_data(), bits );  }
            void adv1 ( )               { oggpackB_adv1  ( this->get_data() );  }
            void reset ( )              { oggpackB_reset ( this->get_data() );  }

            void write (unsigned long value, int bits )     { oggpackB_write ( this->get_data(), value, bits ); }

            void writealign (  )                            { oggpackB_writealign ( this->get_data() ); }
            void writeclear (  )                            { oggpackB_writeclear ( this->get_data() ); }
            void writecopy  ( void *source, long bits )     { oggpackB_writecopy ( this->get_data(), source, bits );  }
            void writetrunc ( long bits )                   { oggpackB_writetrunc ( this->get_data(), bits );  }
    };

    
    /// Basic unit of ogg bitstream format.
    class page {
        private:
            ogg_page data;

        public:
            ogg_page *get_data()        { return &this->data; }

            /// first page in stream?
            bool bos ( )                { return (ogg_page_bos(this->get_data())); }
            bool continued ( )          { return (ogg_page_continued(this->get_data())); }
            /// last page in stream?
            bool eos ( )                { return ogg_page_eos(this->get_data()); }
            /// x.packets() -> number of packets completed on this page
            int packets ( )             { return ogg_page_packets(this->get_data()); }
            int serialno ( )            { return ogg_page_serialno(this->get_data()); }
            int version ( )             { return ogg_page_version(this->get_data()); }
            long pageno ( )             { return ogg_page_pageno(this->get_data()); }
            ogg_int64_t granulepos ( )  { return ogg_page_granulepos(this->get_data()); }
            /// Compute the page's checksum.
            void checksum_set ( )       { ogg_page_checksum_set(this->get_data()); }

            /// for encoding. concatenate header and body to form a page, pages to form ogg streams.
            std::string header ( )      { return std::string((char*)this->data.header, (size_t)this->get_data()->header_len); }
            std::string body ( )        { return std::string((char*)this->data.body, (size_t)this->get_data()->body_len); }
    };

    
    /// Separate raw bitstream into ogg pages.
    class sync {
        protected:
            int   clear     ( )               { return ogg_sync_clear (&this->data); }
            int   destroy   ( )               { return ogg_sync_destroy (&this->data); }
            int   init      ( )               { return ogg_sync_init (&this->data); }
            int   pageout   ( ogg_page *og )  { return ogg_sync_pageout (&this->data, og); }

            /** Skip ahead to beginning of next page.
             * -n means that we skipped n bytes within the bitstream.
             *  0 means that the page isn't ready and we need more data. No bytes have been skipped.
             *  n means that the page was synced at the current location, with a page length of n bytes. 
             */
            long  pageseek  ( ogg_page *og )  { return ogg_sync_pageseek (&this->data, og); }

        public:
            ogg_sync_state data;

            sync ()   { this->init();  }
            ~sync ()  { this->clear(); }

            /// expose a buffer of at least \c size bytes
            char *buffer    ( long size )     { return ogg_sync_buffer (&this->data, size); }
            int   wrote     ( long bytes )    { return ogg_sync_wrote (&this->data, bytes); }

            int   pageout   ( page *og )      { return this->pageout(og->get_data());  }
            long  pageseek  ( page *og )      { return this->pageseek(og->get_data()); }

            /// good to call before seeking
            int   reset     ( )               { return ogg_sync_reset (&this->data); }
    };

    
    /// Stream
    class stream {
        public:
            ogg_stream_state data;

        protected:
            /// allocate non-flat storage for stream
            int init ( int serialno )             { return ogg_stream_init(&this->data, serialno); }
            /// dealloc non-flat storage within
            int clear ( )                         { return ogg_stream_clear(&this->data); }
            /// delete oggvorbis allocated ogg_stream_state structure (would try to _ogg_free this->data)
            int destroy ( )                       { return ogg_stream_destroy(&this->data); }

            int flush ( ogg_page *og )            { return ogg_stream_flush(&this->data, og); }

            int packetin ( ogg_packet *op )       { return ogg_stream_packetin(&this->data, op); }
            int packetout ( ogg_packet *op )      { return ogg_stream_packetout(&this->data, op); }
            int packetpeek ( ogg_packet *op )     { return ogg_stream_packetpeek(&this->data, op); }

            int pagein  ( ogg_page *og )          { return ogg_stream_pagein(&this->data, og); }
            int pageout ( ogg_page *og )          { return ogg_stream_pageout(&this->data, og); }

        public:
            stream(int serialno)                  { this->init(serialno); }
            ~stream()                             { this->clear(); }

            int eos ( )                           { return ogg_stream_eos(&this->data); }

            /// use to flush a packet regardles of its size
            int flush ( page *og )                { return this->flush(og->get_data()); }

            // decode.  copies page in?
            int pagein  ( page *og )              { return this->pagein(og->get_data()); }
            /// decode.  points a packet at an internal data buffer.
            int packetout   ( packet *op )        { return this->packetout(op->get_data()); }
            /** Attempt to assemble a raw data packet and return it without advancing decoding.
             * 
             * Before this method is called, an ogg_page should be submitted to the
             * stream using stream::pagein(). op Pointer to the next packet available
             * in the bitstream, if any. A NULL value may be passed in the case of
             * a simple "is there a packet?" check.
             */
            int packetpeek  ( packet *op )        { return this->packetpeek(op->get_data()); }

            // encode. copies submitted packet. always returns zero.
            int packetin ( packet *op )           { return this->packetin(op->get_data()); }
            int pageout ( page *og )              { return this->pageout(og->get_data()); }

            /// good to call before seeking
            int reset   ( )                       { return ogg_stream_reset(&this->data); }
            int reset_serialno ( int serialno )   { return ogg_stream_reset_serialno(&this->data, serialno); }
    };
};

#endif
