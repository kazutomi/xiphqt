#!/usr/bin/env python
#
# shoutpy's distutils setup script.
#
# Daniel Holth <dholth@fastmail.fm>
# 15 Jan. 2004

from distutils.core import setup
from distutils.extension import Extension

# local configuration
import config

description = """shoutpy provides Pythonic bindings for libshout 2."""

classifiers="""\
Development Status :: 3 - Alpha
License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)
Operating System :: POSIX :: Linux
Programming Language :: C++
Programming Language :: Python
Topic :: Software Development :: Libraries :: Python Modules
Topic :: Multimedia :: Sound/Audio
"""

version="0.5.2"
url = "http://dingoskidneys.com/shoutpy/"

setup(name="shoutpy",
      version=version,
      description="Boost.Python bindings for libshout 2",
      long_description=description,
      author="Daniel Holth",
      author_email="dholth@fastmail.fm",
      url=url,
      download_url=url+"shoutpy-"+version+".tar.gz",
      platforms="linux",
      license="GNU LGPL",
      classifiers=filter(None, classifiers.splitlines()),
      ext_modules=[Extension("shoutpy", ["shoutpy.cc", "shoutcc.cc"],
                             libraries=config.libraries,
                             include_dirs=config.include_dirs,
                             library_dirs=config.library_dirs)])
