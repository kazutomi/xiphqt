
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include <FLAC++/decoder.h>

// Using =======================================================================
using namespace boost::python;

// Declarations ================================================================
namespace  {

struct FLAC_Decoder_Stream_Wrapper: FLAC::Decoder::Stream
{
    FLAC_Decoder_Stream_Wrapper(PyObject* self_):
        FLAC::Decoder::Stream(), self(self_) {}

    FLAC__StreamDecoderReadStatus read_callback(FLAC__byte* p0, unsigned int* p1) {
        return call_method< FLAC__StreamDecoderReadStatus >(self, "read_callback", p0, p1);
    }

    FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame* p0, const FLAC__int32** p1) {
        return call_method< FLAC__StreamDecoderWriteStatus >(self, "write_callback", p0, p1);
    }

    void metadata_callback(const ::FLAC__StreamMetadata* p0) {
        call_method< void >(self, "metadata_callback", p0);
    }

    void error_callback(::FLAC__StreamDecoderErrorStatus p0) {
        call_method< void >(self, "error_callback", p0);
    }

    PyObject* self;
};


}// namespace 


// Module ======================================================================
BOOST_PYTHON_MODULE(flacpy)
{
    scope* FLAC_Decoder_Stream_scope = new scope(
    class_< FLAC_Decoder_Stream_Wrapper >("Stream")
        .def("is_valid", &FLAC::Decoder::Stream::is_valid)
        .def("set_metadata_respond", &FLAC::Decoder::Stream::set_metadata_respond)
        .def("set_metadata_respond_application", &FLAC::Decoder::Stream::set_metadata_respond_application)
        .def("set_metadata_respond_all", &FLAC::Decoder::Stream::set_metadata_respond_all)
        .def("set_metadata_ignore", &FLAC::Decoder::Stream::set_metadata_ignore)
        .def("set_metadata_ignore_application", &FLAC::Decoder::Stream::set_metadata_ignore_application)
        .def("set_metadata_ignore_all", &FLAC::Decoder::Stream::set_metadata_ignore_all)
        .def("get_state", &FLAC::Decoder::Stream::get_state)
        .def("get_channels", &FLAC::Decoder::Stream::get_channels)
        .def("get_channel_assignment", &FLAC::Decoder::Stream::get_channel_assignment)
        .def("get_bits_per_sample", &FLAC::Decoder::Stream::get_bits_per_sample)
        .def("get_sample_rate", &FLAC::Decoder::Stream::get_sample_rate)
        .def("get_blocksize", &FLAC::Decoder::Stream::get_blocksize)
        .def("init", &FLAC::Decoder::Stream::init)
        .def("finish", &FLAC::Decoder::Stream::finish)
        .def("flush", &FLAC::Decoder::Stream::flush)
        .def("reset", &FLAC::Decoder::Stream::reset)
        .def("process_single", &FLAC::Decoder::Stream::process_single)
        .def("process_until_end_of_metadata", &FLAC::Decoder::Stream::process_until_end_of_metadata)
        .def("process_until_end_of_stream", &FLAC::Decoder::Stream::process_until_end_of_stream)
        .def("to_bool", &FLAC::Decoder::Stream::operator bool)
    );

    class_< FLAC::Decoder::Stream::State >("State", init< const FLAC::Decoder::Stream::State& >())
        .def(init< FLAC__StreamDecoderState >())
        .def("as_cstring", &FLAC::Decoder::Stream::State::as_cstring)
        .def("resolved_as_cstring", &FLAC::Decoder::Stream::State::resolved_as_cstring)
        .def("to_FLAC_StreamDecoderState", &FLAC::Decoder::Stream::State::operator FLAC__StreamDecoderState)
    ;

    delete FLAC_Decoder_Stream_scope;

}


