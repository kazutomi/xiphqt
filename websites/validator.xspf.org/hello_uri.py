#! /usr/bin/python
# -----------------------------------------------------------------------
# Online XSPF Validator
# Copyright (C) 2007, Sebastian Pipping / Xiph.Org Foundation
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# Sebastian Pipping, sping@xiph.org
# -----------------------------------------------------------------------

import sys
try:
    from Ft.Lib import Uri
except ImportError:
    print "ERROR: Package 'Ft.Lib' is missing. On Debian testing/unstable run:\n" \
            "sudo apt-get install python-4suite-xml"
    sys.exit(1)

print "Content-Type: text/html"     # HTML is following
print                               # blank line, end of headers


def checkValidity(candidate):
    print "* validUri(\"" + candidate + "\") == " \
            + str(Uri.MatchesUriRefSyntax(candidate)) + "<br>"

def isSafeDownloadTarget(candidate):
    schemeOrNone = Uri.GetScheme(candidate)
    return (schemeOrNone != None) and (schemeOrNone.lower() == "http")

def checkSafety(candidate):
    print "* safeUri(\"" + candidate + "\") == " \
            + str(isSafeDownloadTarget(candidate)) + "<br>"


checkValidity("http://www.xiph.org/")
checkValidity("abc%20def")
checkValidity("abc def")

checkSafety("HTTP://www.example.org/")
checkSafety("ftp://www.example.org/")
