
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <theora/theora.h>
#include <theoracc.h>
#include "theora_wrappers.h"

// Using =======================================================================
using namespace boost::python;

// Module ======================================================================
BOOST_PYTHON_MODULE(theorapy)
{
    class_< ogg::theora::state >("state", init<  >())
        .def(init< const ogg::theora::state& >())
        .def("granule_time", &ogg::theora::state::granule_time)
        .def("decode_init", &ogg::theora::state::decode_init)
        .def("decode_packetin", &ogg::theora::state::decode_packetin)
        .def("decode_YUVout", &ogg::theora::state::decode_YUVout)
        .def("encode_init", &ogg::theora::state::encode_init)
        .def("encode_header", &ogg::theora::state::encode_header)
        .def("encode_tables", &ogg::theora::state::encode_tables)
        .def("encode_YUVin", &ogg::theora::state::encode_YUVin)
        .def("encode_packetout", &ogg::theora::state::encode_packetout)
    ;

    class_< ogg::theora::info >("info", init<  >())
        .def(init< const ogg::theora::info& >())
        .def_readwrite("width", &theora_info::width)
        .def_readwrite("height", &theora_info::height)
        .def_readwrite("frame_width", &theora_info::frame_width)
        .def_readwrite("frame_height", &theora_info::frame_height)
        .def_readwrite("offset_x", &theora_info::offset_x)
        .def_readwrite("offset_y", &theora_info::offset_y)
        .def_readwrite("fps_numerator", &theora_info::fps_numerator)
        .def_readwrite("fps_denominator", &theora_info::fps_denominator)
        .def_readwrite("aspect_numerator", &theora_info::aspect_numerator)
        .def_readwrite("aspect_denominator", &theora_info::aspect_denominator)
        .def_readwrite("colorspace", &theora_info::colorspace)
        .def_readwrite("target_bitrate", &theora_info::target_bitrate)
        .def_readwrite("quality", &theora_info::quality)
        .def_readwrite("quick_p", &theora_info::quick_p)
        .def_readwrite("version_major", &theora_info::version_major)
        .def_readwrite("version_minor", &theora_info::version_minor)
        .def_readwrite("version_subminor", &theora_info::version_subminor)
        .def_readwrite("codec_setup", &theora_info::codec_setup)
        .def_readwrite("dropframes_p", &theora_info::dropframes_p)
        .def_readwrite("keyframe_auto_p", &theora_info::keyframe_auto_p)
        .def_readwrite("keyframe_frequency", &theora_info::keyframe_frequency)
        .def_readwrite("keyframe_frequency_force", &theora_info::keyframe_frequency_force)
        .def_readwrite("keyframe_data_target_bitrate", &theora_info::keyframe_data_target_bitrate)
        .def_readwrite("keyframe_auto_threshold", &theora_info::keyframe_auto_threshold)
        .def_readwrite("keyframe_mindistance", &theora_info::keyframe_mindistance)
        .def_readwrite("noise_sensitivity", &theora_info::noise_sensitivity)
        .def_readwrite("sharpness", &theora_info::sharpness)
        .def("decode_header", &ogg::theora::info::decode_header)
    ;

    class_< ogg::theora::comment >("comment", init<  >())
        .def(init< const ogg::theora::comment& >())
        .def("query", &ogg::theora::comment::query)
        .def("query_index", &ogg::theora::comment::query_index)
        .def("query_count", &ogg::theora::comment::query_count)
        .def("encode", &ogg::theora::comment::encode)
        .def("add", &ogg::theora::comment::add)
        .def("add_tag", &ogg::theora::comment::add_tag)
        .def("get_vendor", &ogg::theora::comment::get_vendor)
        .def("size", &ogg::theora::comment::size)
    ;

    class_< ogg::theora::yuv_image >("yuv_image", init<  >())
        .def(init< const ogg::theora::yuv_image& >())
        .def_readwrite("y_width", &yuv_buffer::y_width)
        .def_readwrite("y_height", &yuv_buffer::y_height)
        .def_readwrite("y_stride", &yuv_buffer::y_stride)
        .def_readwrite("uv_width", &yuv_buffer::uv_width)
        .def_readwrite("uv_height", &yuv_buffer::uv_height)
        .def_readwrite("uv_stride", &yuv_buffer::uv_stride)
        .def_readwrite("y", &yuv_buffer::y)
        .def_readwrite("u", &yuv_buffer::u)
        .def_readwrite("v", &yuv_buffer::v)
        .def("tostrings", &tostrings)
    ;

    def("version_number", &ogg::theora::version_number);
    def("version_string", &ogg::theora::version_string);

    enum_< ogg::theora::theora_errors >("theora_errors")
        .value("ocBadPacket", ogg::theora::ocBadPacket)
        .value("ocNotFormat", ogg::theora::ocNotFormat)
        .value("ocBadHeader", ogg::theora::ocBadHeader)
        .value("ocFault", ogg::theora::ocFault)
        .value("ocEinval", ogg::theora::ocEinval)
        .value("ocImpl", ogg::theora::ocImpl)
        .value("ocNewPacket", ogg::theora::ocNewPacket)
    ;

    enum_< theora_colorspace >("theora_colorspace")
        .value("OC_CS_UNSPECIFIED", OC_CS_UNSPECIFIED)
        .value("OC_CS_ITU_REC_470M", OC_CS_ITU_REC_470M)
        .value("OC_CS_ITU_REC_470BG", OC_CS_ITU_REC_470BG)
    ;

}

