
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include "oggcc.h"
#include "oggpy_wrappers.h"

// Using =======================================================================
using namespace boost::python;

// Declarations ================================================================
namespace  {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ogg_oggpack_base_packetout_overloads_1_3, packetout, 1, 3)


}// namespace 


// Module ======================================================================
BOOST_PYTHON_MODULE(oggpy)
{
    class_< ogg::packet >("packet", init<  >())
        .def(init< const ogg::packet& >())
        .def("packetno", &ogg::packet::packetno)
        .def("granulepos", &ogg::packet::granulepos)
    ;

    class_< ogg::userpacket, bases< ogg::packet >  >("userpacket", init< const ogg::userpacket& >())
        .def(init< std::string, ogg_int64_t, optional< bool, bool > >())
    ;

    class_< ogg::oggpack_base >("oggpack_base", init<  >())
        .def(init< const ogg::oggpack_base& >())
        .def("readinit", (void (ogg::oggpack_base::*)(unsigned char*, int) )&ogg::oggpack_base::readinit)
        .def("readinit", (void (ogg::oggpack_base::*)(ogg::packet*) )&ogg::oggpack_base::readinit)
        .def("writeinit", &ogg::oggpack_base::writeinit)
        .def("packetout", &ogg::oggpack_base::packetout, ogg_oggpack_base_packetout_overloads_1_3())
    ;

    class_< ogg::oggpack, bases< ogg::oggpack_base >  >("oggpack", init<  >())
        .def(init< const ogg::oggpack& >())
        .def("bits", &ogg::oggpack::bits)
        .def("bytes", &ogg::oggpack::bytes)
        .def("look", &ogg::oggpack::look)
        .def("look1", &ogg::oggpack::look1)
        .def("read", &ogg::oggpack::read)
        .def("read1", &ogg::oggpack::read1)
        .def("adv", &ogg::oggpack::adv)
        .def("adv1", &ogg::oggpack::adv1)
        .def("reset", &ogg::oggpack::reset)
        .def("write", &ogg::oggpack::write)
        .def("writealign", &ogg::oggpack::writealign)
        .def("writeclear", &ogg::oggpack::writeclear)
        .def("writecopy", &ogg::oggpack::writecopy)
        .def("writetrunc", &ogg::oggpack::writetrunc)
    ;

    class_< ogg::oggpackB, bases< ogg::oggpack_base >  >("oggpackB", init<  >())
        .def(init< const ogg::oggpackB& >())
        .def("bits", &ogg::oggpackB::bits)
        .def("bytes", &ogg::oggpackB::bytes)
        .def("look", &ogg::oggpackB::look)
        .def("look1", &ogg::oggpackB::look1)
        .def("read", &ogg::oggpackB::read)
        .def("read1", &ogg::oggpackB::read1)
        .def("adv", &ogg::oggpackB::adv)
        .def("adv1", &ogg::oggpackB::adv1)
        .def("reset", &ogg::oggpackB::reset)
        .def("write", &ogg::oggpackB::write)
        .def("writealign", &ogg::oggpackB::writealign)
        .def("writeclear", &ogg::oggpackB::writeclear)
        .def("writecopy", &ogg::oggpackB::writecopy)
        .def("writetrunc", &ogg::oggpackB::writetrunc)
    ;

    class_< ogg::page >("page", init<  >())
        .def(init< const ogg::page& >())
        .def("bos", &ogg::page::bos)
        .def("continued", &ogg::page::continued)
        .def("eos", &ogg::page::eos)
        .def("packets", &ogg::page::packets)
        .def("serialno", &ogg::page::serialno)
        .def("version", &ogg::page::version)
        .def("pageno", &ogg::page::pageno)
        .def("granulepos", &ogg::page::granulepos)
        .def("checksum_set", &ogg::page::checksum_set)
        .def("header", &ogg::page::header)
        .def("body", &ogg::page::body)
    ;

    class_< ogg::sync >("sync", init<  >())
        .def(init< const ogg::sync& >())
        .def("pageout", (int (ogg::sync::*)(ogg::page*) )&ogg::sync::pageout)
        .def("pageseek", (long int (ogg::sync::*)(ogg::page*) )&ogg::sync::pageseek)
        .def("reset", &ogg::sync::reset)
        .def("write", &ogg::sync_write)
    ;

    class_< ogg::stream >("stream", init< const ogg::stream& >())
        .def(init< int >())
        .def("eos", &ogg::stream::eos)
        .def("flush", (int (ogg::stream::*)(ogg::page*) )&ogg::stream::flush)
        .def("pagein", (int (ogg::stream::*)(ogg::page*) )&ogg::stream::pagein)
        .def("packetout", (int (ogg::stream::*)(ogg::packet*) )&ogg::stream::packetout)
        .def("packetpeek", (int (ogg::stream::*)(ogg::packet*) )&ogg::stream::packetpeek)
        .def("packetin", (int (ogg::stream::*)(ogg::packet*) )&ogg::stream::packetin)
        .def("pageout", (int (ogg::stream::*)(ogg::page*) )&ogg::stream::pageout)
        .def("reset", &ogg::stream::reset)
        .def("reset_serialno", &ogg::stream::reset_serialno)
    ;

}

