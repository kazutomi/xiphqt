
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <vorbispy_wrappers.h>

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
BOOST_PYTHON_MODULE(vorbispy)
{
    class_< ogg::vorbis::comment >("comment", init<  >())
        .def(init< const ogg::vorbis::comment& >())
        .def_readwrite("data", &ogg::vorbis::comment::data)
        .def("query", &ogg::vorbis::comment::query)
        .def("query_count", &ogg::vorbis::comment::query_count)
        .def("add", &ogg::vorbis::comment::add)
        .def("add_tag", &ogg::vorbis::comment::add_tag)
        .def("header_out", (int (ogg::vorbis::comment::*)(ogg::packet*) )&ogg::vorbis::comment::header_out)
        .def("query_index", &ogg::vorbis::comment::query_index)
        .def("size", &ogg::vorbis::comment::size)
        .def("get_comments", &get_comments)
    ;

    class_< ogg::vorbis::info >("info", init<  >())
        .def(init< const ogg::vorbis::info& >())
        .def_readwrite("data", &ogg::vorbis::info::data)
        .def("encode_init", &ogg::vorbis::info::encode_init)
        .def("encode_setup_managed", &ogg::vorbis::info::encode_setup_managed)
        .def("encode_setup_vbr", &ogg::vorbis::info::encode_setup_vbr)
        .def("encode_init_vbr", &ogg::vorbis::info::encode_init_vbr)
        .def("encode_ctl", &ogg::vorbis::info::encode_ctl)
        .def("encode_setup_init", &ogg::vorbis::info::encode_setup_init)
        .def("info_blocksize", &ogg::vorbis::info::info_blocksize)
        .def("synthesis_halfrate", &ogg::vorbis::info::synthesis_halfrate)
        .def("synthesis_halfrate_p", &ogg::vorbis::info::synthesis_halfrate_p)
        .def("synthesis_headerin", (int (ogg::vorbis::info::*)(ogg::vorbis::comment*, ogg::packet*) )&ogg::vorbis::info::synthesis_headerin)
        .def("packet_blocksize", (long int (ogg::vorbis::info::*)(ogg::packet*) )&ogg::vorbis::info::packet_blocksize)
        .def("get_version", &ogg::vorbis::info::get_version)
        .def("get_channels", &ogg::vorbis::info::get_channels)
        .def("get_rate", &ogg::vorbis::info::get_rate)
    ;

    class_< ogg::vorbis::block >("block", init<  >())
        .def(init< const ogg::vorbis::block& >())
        .def_readwrite("data", &ogg::vorbis::block::data)
        .def("analysis", (int (ogg::vorbis::block::*)(ogg::packet*) )&ogg::vorbis::block::analysis)
        .def("bitrate_addblock", &ogg::vorbis::block::bitrate_addblock)
        .def("block_clear", &ogg::vorbis::block::block_clear)
        .def("synthesis", (int (ogg::vorbis::block::*)(ogg::packet*) )&ogg::vorbis::block::synthesis)
        .def("synthesis_trackonly", (int (ogg::vorbis::block::*)(ogg::packet*) )&ogg::vorbis::block::synthesis_trackonly)
    ;

    class_< ogg::vorbis::dsp >("dsp", init<  >())
        .def(init< const ogg::vorbis::dsp& >())
        .def_readwrite("data", &ogg::vorbis::dsp::data)
        .def("block_init", (int (ogg::vorbis::dsp::*)(ogg::vorbis::block*) )&ogg::vorbis::dsp::block_init)
        .def("granule_time", &ogg::vorbis::dsp::granule_time)
        .def("bitrate_flushpacket", (int (ogg::vorbis::dsp::*)(ogg::packet*) )&ogg::vorbis::dsp::bitrate_flushpacket)
        .def("analysis_init", (int (ogg::vorbis::dsp::*)(ogg::vorbis::info*) )&ogg::vorbis::dsp::analysis_init)
        .def("analysis_wrote", &ogg::vorbis::dsp::analysis_wrote)
        .def("analysis_blockout", (int (ogg::vorbis::dsp::*)(ogg::vorbis::block*) )&ogg::vorbis::dsp::analysis_blockout)
        .def("analysis_headerout", (int (ogg::vorbis::dsp::*)(ogg::vorbis::comment*, ogg::packet*, ogg::packet*, ogg::packet*) )&ogg::vorbis::dsp::analysis_headerout)
        .def("synthesis_init", (int (ogg::vorbis::dsp::*)(ogg::vorbis::info*) )&ogg::vorbis::dsp::synthesis_init)
        .def("synthesis_blockin", (int (ogg::vorbis::dsp::*)(ogg::vorbis::block*) )&ogg::vorbis::dsp::synthesis_blockin)
        .def("synthesis_lapout", &ogg::vorbis::dsp::synthesis_lapout)
        .def("synthesis_pcmout", &ogg::vorbis::dsp::synthesis_pcmout)
        .def("synthesis_read", &ogg::vorbis::dsp::synthesis_read)
        .def("synthesis_restart", &ogg::vorbis::dsp::synthesis_restart)
        .def("dsp_clear", &ogg::vorbis::dsp::dsp_clear)
        .def("analysis_write", &oggpy_analysis_write)
        .def("synthesis_pcmout", &oggpy_synthesis_pcmout)
    ;

    oggpy_wrappers_init();
}

