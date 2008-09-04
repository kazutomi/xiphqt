#! /usr/bin/python
# coding: utf-8
# -----------------------------------------------------------------------
# Online XSPF Validator
# Copyright (C) 2007-2008, Sebastian Pipping / Xiph.Org Foundation
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
#
# -----------------------------------------------------------------------
# REQUIREMENTS
# -----------------------------------------------------------------------
# * Python 2.4 or later (for current line attribute of Expat parser)
# * Ft.Lib.Uri of 4Suite (http://4suite.org/)
#   (package python-4suite-xml in Debian testing/unstable)
#
# -----------------------------------------------------------------------
# HISTORY
# -----------------------------------------------------------------------
# 2008-09-04 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Fixed: [SECURITY] Accessing local files was possible
#       through using file URIs like file:///etc/passwd
#   * Fixed: [SECURITY] XSS vulnerability existed with URIs like
#       [..]check.py?uri=[..javascript..]
#   * Fixed: [SECURITY] XSS vulnerability existed for
#       certain XSPF input, e.g. in attribute //playlist.version.
#       The input could either come from file upload or URIs like
#       [..]check.py?pasted=[..javascript..]&submitPasted=Submit
#
# 2008-08-25 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Fixed: 'xml:base' attribute now allowed anywhere, was
#       root node only before
#   * Fixed: Error "Attribute '<value>' not allowed" was shown
#       instead of "Attribute '<key>' not allowed".
#
# 2008-07-31 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Fixed: Support for 'xml:base' attribute added
#   * Fixed: Support for XML namespaces added
#   * Fixed: Additional invalid root attributes now rejected
#
# 2007-10-04 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Changed: Link bar updated
#
# 2007-10-03 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Added: Button embedding how-to
#
# 2007-09-24 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Fixed: Another bug in whitespace handling
#   * Added: Finally made testable from the command line
#
# 2007-09-21 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Added: RFC 3986 URI validation
#   * Fixed: Whitespace handling fixes copied over from libSpiff
#   * Changed: Code re-licensed under LGPLv3 (LGPL-Any before) to be
#       able to use 4Suite's Apache-licensed URI validation code
#       (http://www.gnu.org/licenses/lgpl-3.0.html)
#
# 2007-08-09 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Fixed: Ivo's changes repaired to have same look again
#       I use tables when pixel-exact layout is need since
#       CSS support in browsers is not good enough yet
#
# 2007-07-30 -- Ivo Emanuel Gonçalves <justivo gmail.com>
#
#   * Some HTML and CSS fixes
#
# 2007-07-25 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Changes: License changed from GPL to LGPL
#       (http://www.gnu.org/licenses/lgpl-2.1.html)
#
# 2007-02-18 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Added: License header for source code release
#
# 2007-01-20 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Changed: URI checking removed until proper parser available
#
# 2007-01-09 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Fixed: No highlighting on XML error bug fixed
#
# 2006-10-09 -- Sebastian Pipping <webmaster@hartwork.org>
#
#   * Changed: "Valid" now on the top
#
# 2006-10-04 -- Sebastian Pipping <webmaster@hartwork.org>
# ------------------------------------------------------------------------------

import cgi
############## import cgitb; cgitb.enable()
import urllib2
import sys
import xml.parsers.expat
import re

try:
    from Ft.Lib import Uri
except ImportError:
    print "ERROR: Package 'Ft.Lib' is missing. On Debian testing/unstable run:\n" \
            "sudo apt-get install python-4suite-xml"
    sys.exit(2)

print "Content-Type: text/html"     # HTML is following
print                               # blank line, end of headers


# Get the basename of this script
# co = sys._getframe().f_code
# SELFBASE = co.co_filename


def isSafeDownloadTarget(candidate):
    schemeOrNone = Uri.GetScheme(candidate)
    return (schemeOrNone != None) and (schemeOrNone.lower() == "http")


print """
<html lang="en" dir="ltr">
	<head>
		<title>XSPF Validator &mdash; Validate your playlists</title>
		<style type="text/css">
			body {
				background-color:rgb(230,230,230);
				margin:0;
			}

			body, td, h1, h2, h3, h4 {
				font-family:Verdana, sans-serif;
			}

			a {
				color:rgb(130,130,130);
				text-decoration:none;
			}
			
			td.vert {
				background-color:rgb(180,180,180);
				font-size:1pt;
				width:2px;
				padding:0px;
			}
			td.number {
				text-align:right;
				padding-top:3px;
				padding-bottom:3px;
				padding-right:8px;
			}
			a.number {
				color:#000000;
				text-decoration:underline;
			}
			td.error {
				padding-left:8px;
			}
			td.horz {
				background-color:rgb(180,180,180);
				font-size:1px;
				height:2px;
				padding:0;
				line-height:1px;
			}
			span.invalid {
				text-transform:uppercase;
				padding-left:2px;
				color:red;
				font-weight:bold;
			}
			span.valid {
				text-transform:uppercase;
				padding-left:2px;
				color:green;
				font-weight:bold;
			}

			td.lineNumber {
				background-color:rgb(222,222,222);
				font-family:"Courier New", monospace;
				text-align:right;
				vertical-align:top;
				padding-top:3px;
				padding-bottom:3px;
				padding-left:8px;
				padding-right:4px;
			}
			td.lineVert {
					background-color:rgb(0,0,0);
					font-size:1pt;
					width:1px;
					padding:0;
			}
			td.lineOdd {
				font-family:"Courier New", monospace;
				background-color:rgb(236,236,236);
				padding-top:3px;
				padding-bottom:3px;
				padding-left:4px;
				padding-right:8px;
			}
			td.lineEven {
				font-family:"Courier New", monospace;
				background-color:rgb(250,250,250);
				padding-top:3px;
				padding-bottom:3px;
				padding-left:4px;
				padding-right:8px;
			}
			td.lineBad {
				font-family:"Courier New", monospace;
				background-color:rgb(255,188,188);
				padding-top:3px;
				padding-bottom:3px;
				padding-left:4px;
				padding-right:8px;
			}
			
			a.blackLink {
				color:#000000;
				text-decoration:none;
			}
			a.anchor {
				color:#000000;
				text-decoration:none;
			}
		</style>
	</head>
	<body>
		<!-- CENTERING -->
		<table height="100%" style="width:100%">
			<tr>
				<td align="center" valign="middle" style="padding:10px">
					<!-- BORDER -->"""


valid = False
intro = ""
input = ""

shellMode = False
if (len(sys.argv) == 3) and (sys.argv[1] == "--shell"):
    shellMode = True
    try:
        f = open(sys.argv[2])
        try:
            input = f.read()
        finally:
            f.close()
    except IOError:
        pass

    if input != "":
        intro = "Validating local file<br><b><i>" + sys.argv[2] + "</i></b><br><br>"

else:
    form = cgi.FieldStorage()
    if form.has_key("pasted") and form.has_key("submitPasted"):
        input = form.getlist("pasted")[0]
        if input != "":
            intro = "Validating pasted text<br><br>"

    elif form.has_key("uploaded") and form.has_key("submitUploaded"):
        uploaded = form["uploaded"]
        if uploaded.file:
            input = uploaded.file.read()

        if input != "":
            intro = "Validating uploaded file<br><b><i>" + cgi.escape(uploaded.filename) + "</i></b><br><br>"

    elif form.has_key("url"): ### and form.has_key("submitUrl")
        url = form.getlist("url")[0]

        if not isSafeDownloadTarget(url):
            intro = """<b style="color:red;">Download location not considered safe.<br>Please do <em>not</em> attack this site. Thanks.</b><br><br>"""
        else:
            try:
                file = urllib2.urlopen(url)
                input = file.read()
            except ValueError:
                intro = """<b style="color:red;">Invalid URL.</b><br><br>"""

            except urllib2.URLError:
                # One of 404, non-existent host, IPv6 (not supported), ...
                intro = """<b style="color:red">Could not download from URL.</b><br><br>"""

            if input != "":
                intro = "Validating data from URL<br><b><i><a href=\"" + cgi.escape(url, True) \
                        + "\" class=\"blackLink\">" + cgi.escape(url) + "</a></i></b><br><br>"


lineHeads = [0]
if input != "":
    index = -1
    while 1:
        index = input.find("\x0A", index + 1)
        if index == -1:
            break
        lineHeads.append(index + 1)



if input == "":
    # Formular 600
    print """
					<table cellpadding="0" cellspacing="0" width="600" style="border:1px solid rgb(180,180,180); background-color:#FFF;">"""
else:
    # Results 800
    print """
					<table cellpadding="0" cellspacing="0" width="750" style="border:1px solid rgb(180,180,180); background-color:#FFF;">"""

print """
						<tr>"""


if input == "":
    # Formular centered
    print """
							<td align="center" style="padding-top:60px; padding-bottom:60px">
								<form action="" accept-charset="UTF-8" enctype="multipart/form-data" method="post">
								<!-- CONTENT -->
								<table>"""

else:
    # Results full width
    print """
							<td style="padding-top:60px; padding-bottom:50px;">
								<!-- CONTENT -->
								<table style="width:100%;">"""


print """
									<tr>
										<td style="padding-bottom: 20px;" align="center"><img src="xspflogo-1.5.gif" style="width:297px; height:83px; border:0;" alt=""></td>
									</tr>"""


if input == "":
    if intro != "":
        print """
									<tr>
										<td style="width:100%;" align="center">"""
        print intro
        print """
									</tr>"""

    # Formular
    print """
									<tr>
										<td style="padding-bottom:16px;">
											Validate a Spiff playlist from ...
										</td>
									</tr>
									<tr>
										<td>
											URL<br>
											<input name="url" size="60" style="width: 350px;" type="text">
										</td>
									</tr>
									<tr>
										<td style="padding-bottom:20px">
											<input name="submitUrl" value="Submit" type="submit">
										</td>
									</tr>
									<tr>
										<td>
											Uploaded file<br>
											<input name="uploaded" maxlength="100000" accept="text/*" size="41" type="file">
										</td>
									</tr>
									<tr>
										<td style="padding-bottom:20px;">
											<input name="submitUploaded" value="Submit" type="submit">
										</td>
									</tr>
									<tr>
										<td>
											Pasted text<br>
											<textarea cols="60" rows="5" name="pasted" style="width: 350px;"></textarea>
										</td>
									</tr>
									<tr>
										<td>
											<input name="submitPasted" value="Submit" type="submit">
										</td>
									</tr>"""

else:
    print """
									<tr>
										<td style="padding-left:60px; padding-right:60px">"""
    print intro

    stack = []
    valid = True
    version = -1
    accum = ""

    TAG_UNKNOWN = 0
    TAG_PLAYLIST = 1
    TAG_PLAYLIST_TITLE = 2
    TAG_PLAYLIST_CREATOR = 3
    TAG_PLAYLIST_ANNOTATION = 4
    TAG_PLAYLIST_INFO = 5
    TAG_PLAYLIST_LOCATION = 6
    TAG_PLAYLIST_IDENTIFIER = 7
    TAG_PLAYLIST_IMAGE = 8
    TAG_PLAYLIST_DATE = 9
    TAG_PLAYLIST_LICENSE = 10
    TAG_PLAYLIST_ATTRIBUTION = 11
    TAG_PLAYLIST_ATTRIBUTION_LOCATION = 12
    TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER = 13
    TAG_PLAYLIST_LINK = 14
    TAG_PLAYLIST_META = 15
    TAG_PLAYLIST_EXTENSION = 16
    TAG_PLAYLIST_TRACKLIST = 17
    TAG_PLAYLIST_TRACKLIST_TRACK = 18
    TAG_PLAYLIST_TRACKLIST_TRACK_LOCATION = 19
    TAG_PLAYLIST_TRACKLIST_TRACK_IDENTIFIER = 20
    TAG_PLAYLIST_TRACKLIST_TRACK_TITLE = 21
    TAG_PLAYLIST_TRACKLIST_TRACK_CREATOR = 22
    TAG_PLAYLIST_TRACKLIST_TRACK_ANNOTATION = 23
    TAG_PLAYLIST_TRACKLIST_TRACK_INFO = 24
    TAG_PLAYLIST_TRACKLIST_TRACK_IMAGE = 25
    TAG_PLAYLIST_TRACKLIST_TRACK_ALBUM = 26
    TAG_PLAYLIST_TRACKLIST_TRACK_TRACKNUM = 27
    TAG_PLAYLIST_TRACKLIST_TRACK_DURATION = 28
    TAG_PLAYLIST_TRACKLIST_TRACK_LINK = 29
    TAG_PLAYLIST_TRACKLIST_TRACK_META = 30
    TAG_PLAYLIST_TRACKLIST_TRACK_EXTENSION = 31

    skipAbove = -1

    firstPlaylistAnnotation = True
    firstPlaylistAttribution = True
    firstPlaylistCreator = True
    firstPlaylistDate = True
    firstPlaylistIdentifier = True
    firstPlaylistImage = True
    firstPlaylistInfo = True
    firstPlaylistLicense = True
    firstPlaylistLocation = True
    firstPlaylistTitle = True
    firstPlaylistTrackList = True
    firstTrackTitle = True
    firstTrackCreator = True
    firstTrackAnnotation = True
    firstTrackInfo = True
    firstTrackImage = True
    firstTrackAlbum = True
    firstTrackTrackNum = True
    firstTrackDuration = True
    firstTrack = True

    dateRegex = re.compile("^(-?\\d\\d\\d\\d)-(0[1-9]|1[0-2])-(0[1-9]|1[0-9]|2[0-9]|3[0-1])T([0-1][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(\\.\\d+)?([+-](((0[0-9]|1[0-3]):[0-5][0-9])|14:00)|Z)?$")

    SPIFF_NS_HOME = "http://xspf.org/ns/0/"
    SPIFF_NS_SEP_CHAR = " "
    XML_NS_HOME = "http://www.w3.org/XML/1998/namespace"

    checker = xml.parsers.expat.ParserCreate(None, SPIFF_NS_SEP_CHAR)


#############################################################################################


errorTable = ""


def nsXspf(localName):
    return SPIFF_NS_HOME + SPIFF_NS_SEP_CHAR + localName


def nsXml(localName):
    return XML_NS_HOME + SPIFF_NS_SEP_CHAR + localName


def startErrorTable():
    if globals()["errorTable"] == "":
        globals()["errorTable"] += """
											<h3>Error protocol:</h3>
											<table cellspacing="0">
											<tr><td class="number">Line</td><td class="number">Col</td><td class="vert">&nbsp;</td><td class="error">Error</td></tr>
											<tr height="1"><td class="horz">&nbsp;</td><td class="horz">&nbsp;</td><td class="horz">&nbsp;</td><td class="horz">&nbsp;</td></tr>"""


# line is one-based
def addError(line, col, escapedError):
    globals()["errorTable"] += "<tr><td class=\"number\"><a href=\"#bad_" + str(line) + "\" class=\"number\">" + str(line) + "</a></td><td class=\"number\">" + str(col) + "</td><td class=\"vert\">&nbsp;</td><td class=\"error\">" + escapedError + "</td></tr>"


def stopErrorTable():
    if globals()["errorTable"] != "":
        globals()["errorTable"] += """
											</table>
											<br>
											<br>"""

    else:
        globals()["errorTable"] += """
											<br>"""


sourceTable = ""
lastLine = -1


def startSourceTable():
    if globals()["sourceTable"] == "":
        globals()["sourceTable"] = """
											<h3>Processed input:</h3>
											<table cellspacing="0">"""


def stopSourceTable():
    startSourceTable()

    # Add missing lines
    LAST = globals()["lastLine"]
    ALL = len(globals()["lineHeads"])
    if LAST < ALL - 1:
        moreSourceLinesIncluding(ALL - 1, False)
    globals()["sourceTable"] += """
											</table>
											<br>
											<br>"""


# lineNumber is zero-based
def addSourceLine(lineNumber, badFlag):
    globals()["sourceTable"] += """
											<tr>
												<td class="lineNumber">""" + str(lineNumber + 1) + """</td>
												<td class="lineVert">&nbsp;</td>"""
    if badFlag:
        globals()["sourceTable"] += """
												<td class="lineBad"><a name=\"bad_""" + str(lineNumber + 1) + """\" class=\"anchor\">"""
    elif lineNumber % 2:
        globals()["sourceTable"] += """
												<td class="lineEven">"""
    else:
        globals()["sourceTable"] += """
												<td class="lineOdd">"""

    # Last line or normal?
    if lineNumber == len(globals()["lineHeads"]) - 1:
        line = input[lineHeads[lineNumber]:]
    else:
        line = input[lineHeads[lineNumber]:lineHeads[lineNumber + 1] - 1]

    MAX_CHARS_PER_LINE = 100
    line2 = line[0:MAX_CHARS_PER_LINE]
    for i in range(MAX_CHARS_PER_LINE, len(line), MAX_CHARS_PER_LINE):
        line2 += "\n" + line[i:i + MAX_CHARS_PER_LINE]
    globals()["sourceTable"] += cgi.escape(line2).replace("\t", "&nbsp;&nbsp;").replace(" ", "&nbsp;").replace("\n", "<br>")
    if badFlag:
        globals()["sourceTable"] += """</a>"""
    globals()["sourceTable"] += """</td>
											</tr>"""


# lineNumber is zero-based
def moreSourceLinesIncluding(lineNumber, badFlag):
    LAST = globals()["lastLine"]
    if lineNumber <= LAST:
        return

    for i in range(LAST + 1, lineNumber):
        addSourceLine(i, False)
    addSourceLine(lineNumber, badFlag)
    globals()["lastLine"] = lineNumber


def fail(text):
    globals()["valid"] = False

    startErrorTable()
    addError(checker.CurrentLineNumber, checker.CurrentColumnNumber + 1, text)

    startSourceTable()
    moreSourceLinesIncluding(checker.CurrentLineNumber - 1, True)


def handlePlaylistAttribs(atts):
    versionFound = False

    keys = atts.keys()
    for i in range(len(atts)):
        name = keys[i]
        if name == "version":
            dummyVersion = atts.values()[i]
            if dummyVersion == "0":
                globals()["version"] = 0
            elif dummyVersion == "1":
                globals()["version"] = 1
            else:
                fail("Version must be <i>0</i> or <i>1</i>, not '" + cgi.escape(dummyVersion) + "'.")
                globals()["version"] = 1
            versionFound = True
        elif name == nsXml("base"):
            xmlBase = atts.values()[i]
            if not isUri(xmlBase):
                fail("Attribute <i>xml:base</i> is not a URI.")
        else:
            fail("Attribute '" + cgi.escape(name) + "' not allowed.")

    if not versionFound:
        fail("Attribute <i>version</i> missing.")


def handleNoAttribsExceptXmlBase(atts):
    keys = atts.keys()
    for i in range(len(atts)):
        name = keys[i]
        if name == nsXml("base"):
            xmlBase = atts.values()[i]
            if not isUri(xmlBase):
                fail("Attribute <i>xml:base</i> is not a URI.")
        else:
        	fail("Attribute '" + cgi.escape(keys[i]) + "' not allowed.")


def handleExtensionAttribs(atts):
    size = len(atts)
    if size == 0:
        fail("Attribute <i>application</i> missing.")
    else:
        for i in range(size):
            name = atts.keys()[i]
            if name == "application":
                if not isUri(atts.values()[i]):
                    fail("Attribute <i>application</i> is not a URI.")
            elif name == nsXml("base"):
                xmlBase = atts.values()[i]
                if not isUri(xmlBase):
                    fail("Attribute <i>xml:base</i> is not a URI.")
            else:
                fail("Attribute '" + cgi.escape(name) + "' not allowed.")


def handleMetaLinkAttribs(atts):
    size = len(atts)
    if size == 0:
        fail("Attribute <i>rel</i> missing.")
    else:
        for i in range(size):
            name = atts.keys()[i]
            if name == "rel":
                if not isUri(atts.values()[i]):
                    fail("Attribute <i>rel</i> is not a URI.")
            elif name == nsXml("base"):
                xmlBase = atts.values()[i]
                if not isUri(xmlBase):
                    fail("Attribute <i>xml:base</i> is not a URI.")
            else:
                fail("Attribute '" + cgi.escape(name) + "' not allowed.")


def handleStartOne(name, atts):
    if name != nsXspf("playlist"):
        # fail("Element '" + cgi.escape(name) + "' not allowed.")
        fail("Root element must be <i>playlist</i>, not '" + cgi.escape(name) + "'.")
    else:
        handlePlaylistAttribs(atts)
    globals()["stack"].append(TAG_PLAYLIST)


def handleStartTwo(name, atts):
    if name == nsXspf("annotation"):
        if not globals()["firstPlaylistAnnotation"]:
            fail("Only one <i>annotation</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistAnnotation"] = False
        globals()["stack"].append(TAG_PLAYLIST_ANNOTATION)

    elif name == nsXspf("attribution"):
        if not globals()["firstPlaylistAttribution"]:
            fail("Only one <i>attribution</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistAttribution"] = False
        globals()["stack"].append(TAG_PLAYLIST_ATTRIBUTION)

    elif name == nsXspf("creator"):
        if not globals()["firstPlaylistCreator"]:
            fail("Only one <i>creator</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistCreator"] = False
        globals()["stack"].append(TAG_PLAYLIST_CREATOR)

    elif name == nsXspf("date"):
        if not globals()["firstPlaylistDate"]:
            fail("Only one <i>date</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistDate"] = False
        globals()["stack"].append(TAG_PLAYLIST_DATE)

    elif name == nsXspf("extension"):
        if globals()["version"] == 0:
            fail("Element <i>" + cgi.escape(name) + "</i> not allowed in XSPF-0.")
        else:
            handleExtensionAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_EXTENSION)
        # Skip extension body
        globals()["skipAbove"] = 2

    elif name == nsXspf("identifier"):
        if not globals()["firstPlaylistIdentifier"]:
            fail("Only one <i>identifier</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistIdentifier"] = False
        globals()["stack"].append(TAG_PLAYLIST_IDENTIFIER)

    elif name == nsXspf("image"):
        if not globals()["firstPlaylistImage"]:
            fail("Only one <i>image</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistImage"] = False
        globals()["stack"].append(TAG_PLAYLIST_IMAGE)

    elif name == nsXspf("info"):
        if not globals()["firstPlaylistInfo"]:
            fail("Only one <i>info</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistInfo"] = False
        globals()["stack"].append(TAG_PLAYLIST_INFO)

    elif name == nsXspf("license"):
        if not globals()["firstPlaylistLicense"]:
            fail("Only one <i>license</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistLicense"] = False
        globals()["stack"].append(TAG_PLAYLIST_LICENSE)

    elif name == nsXspf("link"):
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_LINK)

    elif name == nsXspf("location"):
        if not globals()["firstPlaylistLocation"]:
            fail("Only one <i>location</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistLocation"] = False
        globals()["stack"].append(TAG_PLAYLIST_LOCATION)

    elif name == nsXspf("meta"):
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_META)

    elif name == nsXspf("title"):
        if not globals()["firstPlaylistTitle"]:
            fail("Only one <i>title</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistTitle"] = False
        globals()["stack"].append(TAG_PLAYLIST_TITLE)

    elif name == nsXspf("trackList"):
        globals()["firstPlaylistTrackList"]
        if not globals()["firstPlaylistTrackList"]:
            fail("Only one <i>trackList</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstPlaylistTrackList"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST)

    else:
        fail("Element <i>" + cgi.escape(name) + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
#        globals()["skipAbove"]
        globals()["skipAbove"] = 2


def handleStartThree(name, atts):
    stackTop = globals()["stack"][len(globals()["stack"]) - 1]
    if stackTop == TAG_PLAYLIST_ATTRIBUTION:
        if name == nsXspf("identifier"):
            handleNoAttribsExceptXmlBase(atts)
            globals()["stack"].append(TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER)

        elif name == nsXspf("location"):
            handleNoAttribsExceptXmlBase(atts)
            globals()["stack"].append(TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER)

        else:
            fail("Element <i>" + cgi.escape(name) + "</i> not allowed.")
            globals()["stack"].append(TAG_UNKNOWN)
            # Skip body of forbidden element
#            globals()["skipAbove"]
            globals()["skipAbove"] = 3

    elif stackTop == TAG_PLAYLIST_TRACKLIST:
        if name == nsXspf("track"):
            handleNoAttribsExceptXmlBase(atts)
            globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK)

        else:
            fail("Element <i>" + cgi.escape(name) + "</i> not allowed.")
            globals()["stack"].append(TAG_UNKNOWN)
            # Skip body of forbidden element
            globals()["skipAbove"] = 3

        globals()["firstTrack"] = False

    else:
        fail("Element <i>" + cgi.escape(name) + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
#        globals()["skipAbove"]
        globals()["skipAbove"] = 3


def handleStartFour(name, atts):
    if name == nsXspf("album"):
        if not globals()["firstTrackAlbum"]:
            fail("Only one <i>album</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackAlbum"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_ALBUM)

    elif name == nsXspf("annotation"):
        if not globals()["firstTrackAnnotation"]:
            fail("Only one <i>annotation</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackAnnotation"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_ANNOTATION)

    elif name == nsXspf("creator"):
        if not globals()["firstTrackCreator"]:
            fail("Only one <i>creator</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackCreator"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_CREATOR)

    elif name == nsXspf("duration"):
        if not globals()["firstTrackDuration"]:
            fail("Only one <i>duration</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackDuration"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_DURATION)

    elif name == nsXspf("extension"):
        if globals()["version"] == 0:
            fail("Element <i>" + cgi.escape(name) + "</i> not allowed in XSPF-0.")
        else:
            handleExtensionAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_EXTENSION)
        # Skip extension body
        globals()["skipAbove"] = 4

    elif name == nsXspf("identifier"):
        handleNoAttribsExceptXmlBase(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_IDENTIFIER)

    elif name == nsXspf("image"):
        if not globals()["firstTrackImage"]:
            fail("Only one <i>image</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackImage"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_IMAGE)

    elif name == nsXspf("info"):
        if not globals()["firstTrackInfo"]:
            fail("Only one <i>info</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackInfo"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_INFO)

    elif name == nsXspf("link"):
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_LINK)

    elif name == nsXspf("location"):
        handleNoAttribsExceptXmlBase(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_LOCATION)

    elif name == nsXspf("meta"):
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_META)

    elif name == nsXspf("trackNum"):
        if not globals()["firstTrackTrackNum"]:
            fail("Only one <i>trackNum</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackTrackNum"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_TRACKNUM)

    elif name == nsXspf("title"):
        if not globals()["firstTrackTitle"]:
            fail("Only one <i>title</i> allowed for <i>track</i>.")
        else:
            handleNoAttribsExceptXmlBase(atts)
        globals()["firstTrackTitle"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_TITLE)

    else:
        fail("Element <i>" + cgi.escape(name) + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
        globals()["skipAbove"] = 4


def handleStart(name, atts):
    newLevel = len(globals()["stack"]) + 1
    if (globals()["skipAbove"] != -1) and (newLevel > globals()["skipAbove"]):
        globals()["stack"].append(TAG_UNKNOWN)
        return

    if newLevel == 1:
        handleStartOne(name, atts)
    elif newLevel == 2:
        handleStartTwo(name, atts)
    elif newLevel == 3:
        handleStartThree(name, atts)
    elif newLevel == 4:
        handleStartFour(name, atts)
    else:
        fail("Element <i>" + cgi.escape(name) + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
        globals()["skipAbove"] = 4


def handleEndOne(name):
    if globals()["firstPlaylistTrackList"]:
        fail("Element <i>trackList</i> missing.")


def handleEndTwo(name):
    stackTop = globals()["stack"][len(globals()["stack"]) - 1]

    # Collapse elements
    # NOTE: whitespace in the middle of <dateTime>,
    # <nonNegativeInteger>, and <anyURI> is illegal anyway
    # which is why we we only cut head and tail here
    if stackTop in [TAG_PLAYLIST_INFO, TAG_PLAYLIST_LOCATION, \
            TAG_PLAYLIST_IDENTIFIER, TAG_PLAYLIST_IMAGE, TAG_PLAYLIST_DATE, \
            TAG_PLAYLIST_LICENSE, TAG_PLAYLIST_LINK, TAG_PLAYLIST_META]:
        globals()["accum"] = globals()["accum"].strip()

    if stackTop == TAG_PLAYLIST_DATE:
        if not isDateTime(globals()["accum"]):
            fail("Content of <i>date</i> is not a dateTime.")

    elif stackTop == TAG_PLAYLIST_IDENTIFIER:
        if not isUri(globals()["accum"]):
            fail("Content of <i>identifier</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_IMAGE:
        if not isUri(globals()["accum"]):
            fail("Content of <i>image</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_INFO:
        if not isUri(globals()["accum"]):
            fail("Content of <i>info</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_LICENSE:
        if not isUri(globals()["accum"]):
            fail("Content of <i>license</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_LINK:
        if not isUri(globals()["accum"]):
            fail("Content of <i>link</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_LOCATION:
        if not isUri(globals()["accum"]):
            fail("Content of <i>location</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST:
        if (globals()["version"] == 0) and (globals()["firstTrack"]):
            fail("Element <i>track</i> missing. This is not allowed in XSPF-0.")

    globals()["accum"] = ""


def handleEndThree(name):
    stackTop = globals()["stack"][len(globals()["stack"]) - 1]

    # Collapse elements
    # NOTE: whitespace in the middle of <dateTime>,
    # <nonNegativeInteger>, and <anyURI> is illegal anyway
    # which is why we we only cut head and tail here
    if stackTop in [TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER, \
            TAG_PLAYLIST_ATTRIBUTION_LOCATION]:
        globals()["accum"] = globals()["accum"].strip()

    if stackTop == TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER:
        if not isUri(globals()["accum"]):
            fail("Content of <i>identifier</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_ATTRIBUTION_LOCATION:
        if not isUri(globals()["accum"]):
            fail("Content of <i>location</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK:
        globals()["firstTrackTitle"] = True
        globals()["firstTrackCreator"] = True
        globals()["firstTrackAnnotation"] = True
        globals()["firstTrackInfo"] = True
        globals()["firstTrackImage"] = True
        globals()["firstTrackAlbum"] = True
        globals()["firstTrackTrackNum"] = True
        globals()["firstTrackDuration"] = True

    globals()["accum"] = ""


def handleEndFour(name):
    stackTop = globals()["stack"][len(globals()["stack"]) - 1]

    # Collapse elements
    # NOTE: whitespace in the middle of <dateTime>,
    # <nonNegativeInteger>, and <anyURI> is illegal anyway
    # which is why we we only cut head and tail here
    if stackTop in [TAG_PLAYLIST_TRACKLIST_TRACK_LOCATION, \
            TAG_PLAYLIST_TRACKLIST_TRACK_IDENTIFIER, \
            TAG_PLAYLIST_TRACKLIST_TRACK_INFO, \
            TAG_PLAYLIST_TRACKLIST_TRACK_IMAGE, \
            TAG_PLAYLIST_TRACKLIST_TRACK_TRACKNUM, \
            TAG_PLAYLIST_TRACKLIST_TRACK_DURATION, \
            TAG_PLAYLIST_TRACKLIST_TRACK_LINK, \
            TAG_PLAYLIST_TRACKLIST_TRACK_META]:
        globals()["accum"] = globals()["accum"].strip()

    if stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_DURATION:
        if not globals()["accum"].isdigit():
            fail("Content of <i>duration</i> is not an unsigned integer.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_IDENTIFIER:
        if not isUri(globals()["accum"]):
            fail("Content of <i>identifier</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_IMAGE:
        if not isUri(globals()["accum"]):
            fail("Content of <i>image</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_INFO:
        if not isUri(globals()["accum"]):
            fail("Content of <i>info</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_LINK:
        if not isUri(globals()["accum"]):
            fail("Content of <i>link</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_LOCATION:
        if not isUri(globals()["accum"]):
            fail("Content of <i>location</i> is not a URI.")

    elif stackTop == TAG_PLAYLIST_TRACKLIST_TRACK_TRACKNUM:
        if not globals()["accum"].isdigit() or (globals()["accum"] == "0"):
            fail("Content of <i>trackNum</i> is not an unsigned integer greater zero.")

    globals()["accum"] = ""


def handleEnd(name):
    level = len(globals()["stack"])
    if globals()["skipAbove"] == level:
        globals()["skipAbove"] = -1
    elif globals()["skipAbove"] > level:
        globals()["stack"].pop()
        return

    if level == 1:
        handleEndOne(name)
    elif level == 2:
        handleEndTwo(name)
    elif level == 3:
        handleEndThree(name)
    elif level == 4:
        handleEndFour(name)

    globals()["stack"].pop()


def handleCharacters(s):
    level = len(globals()["stack"])
    if (globals()["skipAbove"] != -1) and (level > globals()["skipAbove"]):
        return

    stackTop = stack[len(stack) - 1]
    if level == 1:
        if stackTop == TAG_PLAYLIST:
            if s.strip() != "":
                fail("No character data allowed")

    elif level == 2:
        if stackTop in [TAG_PLAYLIST_TRACKLIST, \
                TAG_PLAYLIST_ATTRIBUTION]:
            if s.strip() != "":
                fail("No character data allowed")

        else:
            globals()["accum"] += s

    elif level == 3:
        if (stackTop == TAG_PLAYLIST_TRACKLIST_TRACK):
            if s.strip() != "":
                fail("No character data allowed")

        else:
            globals()["accum"] += s

    elif level == 4:
        globals()["accum"] += s


def isUri(text):
    return Uri.MatchesUriRefSyntax(text)


def isDateTime(text):
    match = globals()["dateRegex"].match(text)
    if not match:
        return False

    # Year- and month-specific day check
    year = int(match.group(1))
    month = int(match.group(2))
    day = int(match.group(3))
    if month == 2:
        if day in [30, 31]:
            return False

        elif day == 29:
            if (((year % 400) != 0) and (((year % 4) != 0) or ((year % 100) == 0))):
                # Not a leap year
                return False;

    elif month in [4, 6, 9, 11]:
        if day > 30:
            return False

    return True


#############################################################################################


if input != "":
    # Results
    checker.StartElementHandler = handleStart
    checker.EndElementHandler = handleEnd
    checker.CharacterDataHandler = handleCharacters

    try:
        checker.Parse(input, 1)
    except xml.parsers.expat.ExpatError:
        errorLineOneBased = checker.ErrorLineNumber
        startErrorTable()
        addError(errorLineOneBased, checker.ErrorColumnNumber, "Invalid XML")
        startSourceTable()
        moreSourceLinesIncluding(errorLineOneBased - 1, True)
        valid = False


    print """
											<h3>Result:</h3>"""

    if valid:
        print """
											<span class="valid">Valid</span>

											<br>
											<br>

											<br>
											
											<h3>Congratulations!</h3>
											<p style="text-align:justify">
											<em>You care about interoperability and that shows: The content you provided is valid XSPF!</em><br>
											<br>
											To show you care and to promote XSPF, you may add this button to any page that serves
											valid XSPF files. Here is HTML code that you can use to add the button:
											</p>
											
											<center>
												<!-- Two elements with horizintal space in between -->
												<table border="0" cellpadding="0" cellspacing="10" style="margin-top:20px;margin-bottom:20px">
													<tr>
														<td>
															<!-- Square border around the the button image -->
															<table border="0" cellpadding="0" cellspacing="1" width="104" height="104" style="background-color:rgb(230,230,230);">
																<tr>
																	<!-- Button image demo -->
																	<td valign="center" align="center" bgcolor="#FFF"><img src="http://svn.xiph.org/websites/xspf.org/images/banners/valid-xspf.png" width="88" height="31" style="border:0" alt="Valid XSPF Playlist" title="This sexy button could promote XSPF on your website!"></td>
																</tr>
															</table>
														</td>
														<!-- Embed code -->
														<td><textarea readonly style="width:410px;height:104px;background-color:rgb(250,250,250);"><a href="http://validator.xspf.org/referrer/"><img src="valid-xspf.png" width="88" height="31" style="border:0" alt="Valid XSPF Playlist" title="This website produces valid XSPF playlist files."></a></textarea></td>
													</tr>
												</table>
											</center>

											Well done, please come back soon!"""
    else:
        print """
											<span class="invalid">Invalid</span>"""
											
											

    print """
											<br>
											<br>"""


    stopErrorTable()
    print errorTable

    stopSourceTable()
    print sourceTable


    print """	
										</td>
									</tr>
								</table>"""

else:
    # Formular
    print """	
								</table>
								</form>"""


print """
							</td>
						</tr>
						<tr>
							<td align="center" style="padding-left:6px; padding-right:6px;">
								<table cellpadding="0" cellspacing="0" height="1" style="width:100%; background-color:rgb(180,180,180);">
									<tr>
										<td style="font-size:1px; line-height:1px;">&nbsp;</td>
									</tr>
								</table>
							</td>
						</tr>
						<tr>
							<td align="center" style="font-size:9pt; padding-top:2px; padding-bottom:5px;">
								<a href="https://trac.xiph.org/browser/websites/validator.xspf.org">Source Code</a>&nbsp;&nbsp;&nbsp;
								<a href="https://trac.xiph.org/browser/trunk/xspf/testcase">Test Cases</a>&nbsp;&nbsp;&nbsp;
								<a href="http://libspiff.sourceforge.net/">libSpiff</a>&nbsp;&nbsp;&nbsp;
								<a href="http://xspf.org/xspf-v1.html">XSPF Spec</a>&nbsp;&nbsp;&nbsp;
							</td>
						</tr>
					</table>
				</td>
			</tr>
		</table>
	</body>
</html>"""


if shellMode:
    if valid:
        sys.exit(0)

    else:
        sys.exit(1)
