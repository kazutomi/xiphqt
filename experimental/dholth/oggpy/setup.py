#!/usr/bin/env python
# oggpy 

from distutils.core import setup
from distutils.extension import Extension

# edit to point to your boost::python installation
boost = "/home/dholth/opt/include/boost-1_31"

description = """oggpy provides alternative python bindings for ogg."""

classifiers="""\
Development Status :: 3 - Alpha
License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)
Operating System :: POSIX :: Linux
Programming Language :: C++
Programming Language :: Python
Topic :: Software Development :: Libraries :: Python Modules
Topic :: Multimedia :: Sound/Audio
"""

setup(name="oggpy",
      version="0.0",
      description="Boost.Python interface for ogg",
      long_description=description,
      author="Daniel Holth",
      author_email="dholth@fastmail.fm",
      url="http://dingoskidneys.com/shoutpy/",
      platforms="linux",
      license="GNU LGPL",
      classifiers=filter(None, classifiers.splitlines()),
      ext_modules=[Extension("oggpy", ["oggcc.cc", "oggpy.cpp"],
                             libraries=["ogg", "vorbis", "boost_python-gcc"],
                             include_dirs=[boost])])
