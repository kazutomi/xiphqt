# Local configuration for ogpgy.
# Daniel Holth <dholth@fastmail.fm>, 2004
# Works in gentoo.

common_libraries = ["ogg"]
common_libraries.extend(("boost_python",))
# libraries.extend(("boost_python-gcc-mt-1_31",)) # this name may also work for you

common_include_dirs = ["."]

oggpy_libraries = common_libraries
oggpy_include_dirs = common_include_dirs

vorbispy_libraries = common_libraries + ["vorbis", "vorbisenc"]
vorbispy_include_dirs = common_include_dirs

theorapy_libraries = common_libraries + ["theora"]
theorapy_include_dirs = common_include_dirs

bridge_libraries = common_libraries + ["SDL", "theora"]
bridge_include_dirs = common_include_dirs + ["/usr/include/SDL"]
