#!/usr/bin/env python
#
# shoutpy's distutils setup script.
#
# Daniel Holth <dholth@fastmail.fm>
# 15 Jan. 2004 - 26 Nov. 2006

try:
    from setuptools import setup
    from setuptools.extension import Extension
except:
    from distutils.core import setup
    from distutils.extension import Extension

# local configuration
import config

description = """shoutpy provides Pythonic bindings for libshout 2.1 or higher"""

classifiers="""\
Development Status :: 4 - Beta
License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)
Operating System :: POSIX :: Linux
Programming Language :: C++
Programming Language :: Python
Topic :: Software Development :: Libraries :: Python Modules
Topic :: Multimedia :: Sound/Audio
"""

version="1.0.0"
url = "http://dingoskidneys.com/shoutpy/"

setup(name="shoutpy",
      version=version,
      description="Boost.Python bindings for libshout >= 2.1",
      long_description=description,
      author="Daniel Holth",
      author_email="dholth@fastmail.fm",
      url=url,
      download_url=url+"shoutpy-"+version+".tar.gz",
      platforms="linux",
      license="GNU LGPL",
      keywords="shout libshout icecast ogg streaming audio",
      classifiers=filter(None, classifiers.splitlines()),
      ext_modules=[Extension("shoutpy", ["shoutpy.cc", "shoutcc.cc"],
                             libraries=config.libraries,
                             include_dirs=config.include_dirs,
                             library_dirs=config.library_dirs)])
