#!/usr/bin/env python
# oggpy 

from distutils.core import setup
from distutils.extension import Extension
import os.path
import sys

extra_include_dirs = [".", "/home/dholth/include/theora", "/usr/include/SDL"]

if sys.version[0:3] == "2.3":
    boost = "/home/dholth/opt/include/boost-1_31" # boost.python include path
    boostlib = "boost_python-gcc-1_31"            # boost.python library name
else:
    boost  = "/usr/include/boost"
    boostlib = "boost_python"

if(os.path.exists(boost)):
    extra_include_dirs.append(boost)

description = """oggpy provides alternative python bindings to ogg, vorbis and theora."""

classifiers="""\
Development Status :: 2 - Pre-Alpha
License :: OSI Approved :: MIT License
Operating System :: POSIX :: Linux
Programming Language :: C++
Programming Language :: Python
Topic :: Software Development :: Libraries :: Python Modules
Topic :: Multimedia :: Sound/Audio
"""

setup(name="oggpy",
      version="0.4",
      description="Boost.Python and C++ wrappers of ogg, vorbis and theora",
      long_description=description,
      author="Daniel Holth",
      author_email="dholth@fastmail.fm",
      url="http://dingoskidneys.com/oggpy/",
      platforms="linux",
      license="MIT",
      classifiers=filter(None, classifiers.splitlines()),

      ext_modules=[
                    Extension("oggpy", ["oggcc.cc", "oggpy.cpp", "oggpy_wrappers.cc"],
                              libraries=["ogg", boostlib],
                              include_dirs=extra_include_dirs, depends=["oggcc.h"]),
      
                    Extension("vorbispy", ["oggcc.cc", "vorbispy.cpp", "vorbispy_wrappers.cc"],
                              libraries=["ogg", "vorbis", "vorbisenc", boostlib],  
                              include_dirs=extra_include_dirs, depends=["oggcc.h", "vorbiscc.h", "vorbispy_wrappers.h"]),
                   
                    Extension("theorapy", ["oggcc.cc", "theorapy.cpp"],
                              libraries=["ogg", "theora", boostlib],
                              include_dirs=extra_include_dirs, depends=["oggcc.h", "theoracc.h"])

# If we want to statically link libboost_python we would have to put
# everything in one file; this doesn't work right away due to name
# collisions.
#                    , Extension("panogg", 
#                              ["panogg.cc", "oggpy/_oggpy.cpp", "oggpy/_theorapy.cpp", 
#                              "oggpy/_vorbispy.cpp", "vorbispy_wrappers.cc", "oggpy_wrappers.cc"],
#                              libraries=["ogg", "theora", "vorbis", "vorbisenc", boostlib],
#                              include_dirs=extra_include_dirs, depends=["oggcc.h", "theoracc.h"])
                    # this is broken too; I think the pygame <--> boost connection is not all there.
                    # Extension("pygame_bridge", ["pygame_bridge.cc"],
                    #     libraries=["SDL", "ogg", "theora", boostlib],
                    #     include_dirs=extra_include_dirs)

                    ]
     )

