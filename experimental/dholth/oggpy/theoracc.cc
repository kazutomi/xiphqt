#include "theoracc.h"

namespace ogg {
    namespace theora {
        /// Human-readable version of libtheora
        const char *version_string  ( void )    { return  theora_version_string ( ); }
        /// Computer-convenient numeric version of libtheora
        ogg_uint32_t version_number ( void )    { return  theora_version_number ( ); }
    };
};
