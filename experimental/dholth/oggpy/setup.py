#!/usr/bin/env python
# oggpy, by Daniel Holth <dholth@fastmail.fm>, 2004

from distutils.core import setup
from distutils.extension import Extension
import os.path
import sys

# local configuration
import config

description = """oggpy provides capable python bindings of ogg, vorbis and theora."""

classifiers="""\
Development Status :: 2 - Pre-Alpha
License :: OSI Approved :: MIT License
Operating System :: POSIX :: Linux
Programming Language :: C++
Programming Language :: Python
Topic :: Software Development :: Libraries :: Python Modules
Topic :: Multimedia :: Sound/Audio
Topic :: Multimedia :: Video
"""

version = "0.5"
url = "http://dingoskidneys.com/oggpy/"

setup(name="oggpy",
      version=version,
      description="Boost.Python and C++ wrappers of ogg, vorbis and theora",
      long_description=description,
      author="Daniel Holth",
      author_email="dholth@fastmail.fm",
      url=url,
      download_url=url+"oggpy-"+version+".tar.gz",
      platforms="linux",
      license="MIT",
      classifiers=filter(None, classifiers.splitlines()),

      ext_modules=[
                    Extension("oggpy", ["oggcc.cc", "oggpy.cpp", "oggpy_wrappers.cc"],
                              libraries=config.oggpy_libraries,
                              include_dirs=config.oggpy_include_dirs,
                              depends=["oggcc.h"]),
      
                    Extension("vorbispy", ["oggcc.cc", "vorbispy.cpp", "vorbispy_wrappers.cc"],
                              libraries=config.vorbispy_libraries,  
                              include_dirs=config.vorbispy_include_dirs,
                              depends=["oggcc.h", "vorbiscc.h", "vorbispy_wrappers.h"]),
                   
                    Extension("theorapy", ["oggcc.cc", "theorapy.cpp"],
                              libraries=config.theorapy_libraries,
                              include_dirs=config.theorapy_include_dirs,
                              depends=["oggcc.h", "theoracc.h"]),
                    
                    Extension("flacpy", ["flacpy.cpp"],
                              libraries=config.flacpy_libraries,
                              include_dirs=config.flacpy_include_dirs)

                    # It works!
                    , Extension("pygame_bridge", ["pygame_bridge.cc"],
                                libraries=config.bridge_libraries,
                                include_dirs=config.bridge_include_dirs)

                    ]
     )

