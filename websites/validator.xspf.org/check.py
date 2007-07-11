#!/usr/bin/python
# ------------------------------------------------------------------------------
# Written by Sebastian Pipping <webmaster@hartwork.org>
# Licensed under GNU General Public License http://www.gnu.org/copyleft/gpl.html
#
# NOTE: Python 2.4 or later requiered for current line attribute of Expat parser
#
# 2007-02-18
#   * Added: License header for source code release
# 2007-01-20
#   * Changed: URI checking removed until proper parser available
# 2007-01-09
#   * Fixed: No highlighting on XML error bug fixed
# 2006-10-09
#   * Changed: "Valid" now on the top
# 2006-10-04
# ------------------------------------------------------------------------------

import cgi
############## import cgitb; cgitb.enable()
import urllib2
import sys
import xml.parsers.expat
import re

print "Content-Type: text/html"     # HTML is following
print                               # blank line, end of headers



# Get the basename of this script
# co = sys._getframe().f_code
# SELFBASE = co.co_filename



print """
<html>
	<head>
		<title>XSPF Validator</title>
		<style>
			body {
				background-color:rgb(230,230,230);
				margin:0px;
			}

			body, td, h1, h2, h3, h4 {
				font-family:"Verdana";
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
				padding:0px;
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
				font-family:"Courier new";
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
					padding:0px;
			}
			td.lineOdd {
				font-family:"Courier new";
				background-color:rgb(236,236,236);
				padding-top:3px;
				padding-bottom:3px;
				padding-left:4px;
				padding-right:8px;
			}
			td.lineEven {
				font-family:"Courier new";
				background-color:rgb(250,250,250);
				padding-top:3px;
				padding-bottom:3px;
				padding-left:4px;
				padding-right:8px;
			}
			td.lineBad {
				font-family:"Courier new";
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
		<table width="100%" height="100%">
			<tr>
				<td align="center" valign="center" style="padding:10px">
					<!-- BORDER -->"""


intro = ""

input = ""
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
        intro = "Validating uploaded file<br><b><i>" + uploaded.filename + "</i></b><br><br>"

elif form.has_key("url"): ### and form.has_key("submitUrl")
    url = form.getlist("url")[0]

    try:
        file = urllib2.urlopen(url)
        input = file.read()
    except ValueError:
        intro = """<b style="color:red">Invalid URL.</b><br><br>"""

    except urllib2.HTTPError:
        # 404 or similar
        intro = """<b style="color:red">Could not download from URL.</b><br><br>"""

    if input != "":
        intro = "Validating data from URL<br><b><i><a href=\"" + url + "\" target=\"_blank\" class=\"blackLink\">" + url + "</a></i></b><br><br>"



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
					<table cellpadding="0" cellspacing="0" width="600" style="border:1px solid rgb(180,180,180); background-color:white">"""
else:
    # Results 800
    print """
					<table cellpadding="0" cellspacing="0" width="750" style="border:1px solid rgb(180,180,180); background-color:white">"""

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
							<td style="padding-top:60px; padding-bottom:50px">
								<!-- CONTENT -->
								<table width="100%">"""



print """
									<tr>
										<td style="padding-bottom: 20px;" align="center"><img src="xspflogo-1.5.gif" width="297" height="83" border="0" /></td>
									</tr>"""



if input == "":
    if intro != "":
        print """
									<tr>
										<td width="100%" align="center">"""
        print intro
        print """
									</tr>"""

    # Formular
    print """
									<tr>
										<td style="padding-bottom:16px">
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
										<td style="padding-bottom:20px">
											<input name="submitUploaded" value="Submit" type="submit">
										</td>
									</tr>
									<tr>
										<td>
											Pasted text<br>
											<textarea cols="60" rows="5" name="pasted" size="60" style="width: 350px;"></textarea>
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

    uriRegex = re.compile("^[a-zA-Z0-9.+-]+:")
    dateRegex = re.compile("^(-?\\d\\d\\d\\d)-(0[1-9]|1[0-2])-(0[1-9]|1[0-9]|2[0-9]|3[0-1])T([0-1][0-9]|2[0-3]):([0-5][0-9]):([0-5][0-9])(\\.\\d+)?([+-](((0[0-9]|1[0-3]):[0-5][0-9])|14:00)|Z)?$")

    checker = xml.parsers.expat.ParserCreate()



#############################################################################################


errorTable = ""


def startErrorTable():
    if globals()["errorTable"] == "":
        globals()["errorTable"] += """
											<h3>Error protocol:</h3>
											<table cellspacing="0">
											<tr><td class="number">Line</td><td class="number">Col</td><td class="vert">&nbsp;</td><td class="error">Error</td></tr>
											<tr height="1"><td class="horz">&nbsp;</td><td class="horz">&nbsp;</td><td class="horz">&nbsp;</td><td class="horz">&nbsp;</td></tr>"""



# line is one-based
def addError(line, col, error):
    globals()["errorTable"] += "<tr><td class=\"number\"><a href=\"#bad_" + str(line) + "\" class=\"number\">" + str(line) + "</a></td><td class=\"number\">" + str(col) + "</td><td class=\"vert\">&nbsp;</td><td class=\"error\">" + error + "</td></tr>"



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
    globals()["sourceTable"] += line2.replace("<", "&lt;").replace(">", "&gt;").replace("\t", "&nbsp;&nbsp;").replace(" ", "&nbsp;").replace("\n", "<br>")
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
    xmlnsFound = False

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
                fail("Version must be <i>0</i> or <i>1</i>, not '" + dummyVersion + "'.")
                globals()["version"] = 1
            versionFound = True
        elif name == "xmlns":
            namespace = atts.values()[i]
            if namespace != "http://xspf.org/ns/0/":
                fail("Namespace must be 'http://xspf.org/ns/0/', not '" + namespace + "'.")
            xmlnsFound = True
            
    if not versionFound:
        fail("Attribute <i>version</i> missing.")
    if not xmlnsFound:
        fail("Attribute <i>xmlns</i> missing.")



def handleNoAttribs(atts):
    keys = atts.keys()
    for i in range(len(atts)):
        fail("Attribute '" + atts.values()[i] + "' not allowed.")
    


def handleExtensionAttribs(atts):
    size = len(atts)
    if size == 0:
        fail("Attribute <i>application</i> missing.")
    else:
        for i in range(size):
            name = atts.keys()[i]
            if name != "application":
                fail("Attribute '" + name + "' not allowed.")
            elif not isUri(atts.values()[i]):
                fail("Attribute <i>application</i> is not a URI.")
                


def handleMetaLinkAttribs(atts):
    size = len(atts)
    if size == 0:
        fail("Attribute <i>rel</i> missing.")
    else:
        for i in range(size):
            name = atts.keys()[i]
            if name != "rel":
                fail("Attribute '" + name + "' not allowed.")
            elif not isUri(atts.values()[i]):
                fail("Attribute <i>rel</i> is not a URI.")



def handleStartOne(name, atts):
    if name != "playlist":
        # fail("Element '" + name + "' not allowed.")
        fail("Root element must be <i>playlist</i>, not '" + name + "'.")
    else:
        handlePlaylistAttribs(atts)
    globals()["stack"].append(TAG_PLAYLIST)



def handleStartTwo(name, atts):
    if name == "annotation":
        if not globals()["firstPlaylistAnnotation"]:
            fail("Only one <i>annotation</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistAnnotation"] = False
        globals()["stack"].append(TAG_PLAYLIST_ANNOTATION)

    elif name == "attribution":
        if not globals()["firstPlaylistAttribution"]:
            fail("Only one <i>attribution</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistAttribution"] = False
        globals()["stack"].append(TAG_PLAYLIST_ATTRIBUTION)

    elif name == "creator":
        if not globals()["firstPlaylistCreator"]:
            fail("Only one <i>creator</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistCreator"] = False
        globals()["stack"].append(TAG_PLAYLIST_CREATOR)

    elif name == "date":
        if not globals()["firstPlaylistDate"]:
            fail("Only one <i>date</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistDate"] = False
        globals()["stack"].append(TAG_PLAYLIST_DATE)

    elif name == "extension":
        if globals()["version"] == 0:
            fail("Element <i>" + name + "</i> not allowed in XSPF-0.")
        else:
            handleExtensionAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_EXTENSION)
        # Skip extension body
        globals()["skipAbove"] = 2

    elif name == "identifier":
        if not globals()["firstPlaylistIdentifier"]:
            fail("Only one <i>identifier</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistIdentifier"] = False
        globals()["stack"].append(TAG_PLAYLIST_IDENTIFIER)

    elif name == "image":
        if not globals()["firstPlaylistImage"]:
            fail("Only one <i>image</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistImage"] = False
        globals()["stack"].append(TAG_PLAYLIST_IMAGE)

    elif name == "info":
        if not globals()["firstPlaylistInfo"]:
            fail("Only one <i>info</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistInfo"] = False
        globals()["stack"].append(TAG_PLAYLIST_INFO)

    elif name == "license":
        if not globals()["firstPlaylistLicense"]:
            fail("Only one <i>license</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistLicense"] = False
        globals()["stack"].append(TAG_PLAYLIST_LICENSE)

    elif name == "link":
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_LINK)

    elif name == "location":
        if not globals()["firstPlaylistLocation"]:
            fail("Only one <i>location</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistLocation"] = False
        globals()["stack"].append(TAG_PLAYLIST_LOCATION)

    elif name == "meta":
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_META)

    elif name == "title":
        if not globals()["firstPlaylistTitle"]:
            fail("Only one <i>title</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistTitle"] = False
        globals()["stack"].append(TAG_PLAYLIST_TITLE)

    elif name == "trackList":
        globals()["firstPlaylistTrackList"]
        if not globals()["firstPlaylistTrackList"]:
            fail("Only one <i>trackList</i> allowed for <i>playlist</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstPlaylistTrackList"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST)

    else:
        fail("Element <i>" + name + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
#        globals()["skipAbove"]
        globals()["skipAbove"] = 2



def handleStartThree(name, atts):
    stackTop = globals()["stack"][len(globals()["stack"]) - 1]
    if stackTop == TAG_PLAYLIST_ATTRIBUTION:
        if name == "identifier":
            handleNoAttribs(atts)
            globals()["stack"].append(TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER)

        elif name == "location":
            handleNoAttribs(atts)
            globals()["stack"].append(TAG_PLAYLIST_ATTRIBUTION_IDENTIFIER)

        else:        
            fail("Element <i>" + name + "</i> not allowed.")
            globals()["stack"].append(TAG_UNKNOWN)
            # Skip body of forbidden element
#            globals()["skipAbove"]
            globals()["skipAbove"] = 3

    elif stackTop == TAG_PLAYLIST_TRACKLIST:
        if name == "track":
            handleNoAttribs(atts)
            globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK)

        else:
            fail("Element <i>" + name + "</i> not allowed.")
            globals()["stack"].append(TAG_UNKNOWN)
            # Skip body of forbidden element
            globals()["skipAbove"] = 3
            
        globals()["firstTrack"] = False

    else:
        fail("Element <i>" + name + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
#        globals()["skipAbove"]
        globals()["skipAbove"] = 3

        

def handleStartFour(name, atts):
    if name == "album":
        if not globals()["firstTrackAlbum"]:
            fail("Only one <i>album</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackAlbum"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_ALBUM)

    elif name == "annotation":
        if not globals()["firstTrackAnnotation"]:
            fail("Only one <i>annotation</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackAnnotation"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_ANNOTATION)

    elif name == "creator":
        if not globals()["firstTrackCreator"]:
            fail("Only one <i>creator</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackCreator"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_CREATOR)

    elif name == "duration":
        if not globals()["firstTrackDuration"]:
            fail("Only one <i>duration</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackDuration"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_DURATION)

    elif name == "extension":
        if globals()["version"] == 0:
            fail("Element <i>" + name + "</i> not allowed in XSPF-0.")
        else:
            handleExtensionAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_EXTENSION)
        # Skip extension body
        globals()["skipAbove"] = 4
        
    elif name == "identifier":
        handleNoAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_IDENTIFIER)

    elif name == "image":
        if not globals()["firstTrackImage"]:
            fail("Only one <i>image</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackImage"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_IMAGE)

    elif name == "info":
        if not globals()["firstTrackInfo"]:
            fail("Only one <i>info</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackInfo"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_INFO)

    elif name == "link":
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_LINK)

    elif name == "location":
        handleNoAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_LOCATION)

    elif name == "meta":
        handleMetaLinkAttribs(atts)
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_META)

    elif name == "trackNum":
        if not globals()["firstTrackTrackNum"]:
            fail("Only one <i>trackNum</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackTrackNum"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_TRACKNUM)

    elif name == "title":
        if not globals()["firstTrackTitle"]:
            fail("Only one <i>title</i> allowed for <i>track</i>.")
        else:
            handleNoAttribs(atts)
        globals()["firstTrackTitle"] = False
        globals()["stack"].append(TAG_PLAYLIST_TRACKLIST_TRACK_TITLE)

    else:
        fail("Element <i>" + name + "</i> not allowed.")
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
        fail("Element <i>" + name + "</i> not allowed.")
        globals()["stack"].append(TAG_UNKNOWN)
        # Skip body of forbidden element
        globals()["skipAbove"] = 4



def handleEndOne(name):
    if globals()["firstPlaylistTrackList"]:
        fail("Element <i>trackList</i> missing.")



def handleEndTwo(name):
    stackTop = globals()["stack"][len(globals()["stack"]) - 1]
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
        if stackTop in [TAG_PLAYLIST_TRACKLIST, TAG_PLAYLIST_ATTRIBUTION]:
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
        
    else:
        globals()["accum"] += s



def isUri(text):
    return True # 2007-01-20, TODO, OLD: globals()["uriRegex"].match(text)



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
											<span class="valid">Valid</span>"""
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
							<td align="center" style="padding-left:6px; padding-right:6px">
								<table cellpadding="0" cellspacing="0" width="100%" height="1" style="background-color:rgb(180,180,180)">
									<tr>
										<td style="font-size:1px; line-height:1px">&nbsp;</td>
									</tr>
								</table>
							</td>
						<tr>
						<tr>
							<td align="center" style="font-size:9pt; padding-top:2px; padding-bottom:5px">
								<a href="http://www.hartwork.org/" target="_blank">Hartwork Project</a>&nbsp;&nbsp;&nbsp;
								<a href="http://libspiff.sourceforge.net/" target="_blank">libSpiff</a>&nbsp;&nbsp;&nbsp;
								<a href="http://www.xspf.org/specs/" target="_blank">XSPF Spec</a>&nbsp;&nbsp;&nbsp;
							</td>
						</tr>
					</table>
				</td>
			</tr>
		</table>
	</body>
</html>"""
