# © 2004 Nathan Sharfi
#
# This software is provided 'as-is', without any express or implied warranty.
# In no event will the authors be held liable for any damages arising from
# the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
#    1. The origin of this software must not be misrepresented; you must not
#       claim that you wrote the original software. If you use this software
#       in a product, an acknowledgment in the product documentation would be
#       appreciated but is not required.
#
#    2. Altered source versions must be plainly marked as such, and must not
#       be misrepresented as being the original software.
#
#    3. This notice may not be removed or altered from any source distribution.

import os, re, sys, glob, os.path
MARKDOWN_COMMAND = "Markdown" # could also be "Markdown.pl"
TITLE_REGEX = re.compile(r"<h1>(.*)</h1>")
class MarkdownWrapper(object):
	"""
	Important attributes:
	
		.sourceFilename		-- the file containing markdown text
		.destFilename		-- the output html file name
		.doctype			-- self explanatory
		.importables		-- a list of CSS files to @import
		.inHeadBits			-- a list of strings to be included inside the head element
		.beforeContentBits	-- stuff to include before the content div
		.mainText			-- the text from .sourceFileName after it's been turned into HTML
		.afterContentBits	-- stuff to include after the content div
	"""
	def __init__(self, sourceFilename, destFilename=None):
		self.sourceFilename = sourceFilename
		if destFilename:
			self.destFilename = destFilename
		else:
			self.destFilename = os.path.splitext(sourceFilename)[0] + ".html"
		self.doctype = '<!DOCTYPE HTML PUBLIC ' + \
					   '"-//W3C//DTD HTML 4.01//EN" ' + \
					   '"http://www.w3.org/TR/html4/strict.dtd">'
		
		self.importables = "/CSS/all.css".split()
		
		self.mainText = ""
		fh = os.popen(MARKDOWN_COMMAND + " " + sourceFilename)
		try:
			self.mainText = fh.read()
		finally:
			fh.close()

		match = TITLE_REGEX.match(self.mainText)
		self.title = "Untitled Document"
		if match:
			self.title = match.group(1)
		
		self.lang = 'en-US'

		self.headBits = []
		self.beforeContentBits = []
		self.afterContentBits = []

		sidebarFilename = os.path.join(os.path.split(self.sourceFilename)[0], "sidebar.inc")
		if os.path.isfile(sidebarFilename):
			# consider crawling back to ./ from ./foo/bar/baz/quux
			beforeBit = "<div id='sidebar'>"
			beforeBit += (file(sidebarFilename).read())
			beforeBit += "</div>"
			self.beforeContentBits.append(beforeBit)
			
		
		# todo: similarly to append, say, copyright.inc to afterContentBits

	def renderToFile(self):
		outfile = file(self.destFilename, "w")
		try:
			outfile.write(self.renderToString())
		finally:
			outfile.close()
		

	def renderToString(self):
		"""dump self to string."""
		ret = ""
		ret += self.doctype + '\n'
		ret += "<html lang='" + self.lang + "'>\n"
		ret += "<head>\n"
		ret += '<meta http-equiv="Content-Type" content="text/html; charset=utf-8">\n'
		ret += '<title>' + self.title + '</title>\n'
		for importable in self.importables:
			ret += "<style type='text/css'>@import url(" + importable + ");</style>\n"
		for bit in self.headBits:
			ret += bit
		ret += "</head>\n"
		ret += "<body>\n"
		for bit in self.beforeContentBits:
			ret += bit
		ret += "<div id='content'>"
		ret += self.mainText
		ret += "</div>\n"
		for bit in self.afterContentBits:
			ret += bit
		ret += "</body>\n"
		ret += "</html>\n"
		return ret

if __name__ == '__main__':
	for dirpath, dirnames, filenames in os.walk('.'):
		for filename in filter(lambda x: x.endswith(".txt"), filenames):
			fullpath = os.path.join(dirpath, filename)
			print fullpath
			mw = MarkdownWrapper(fullpath)
			mw.renderToFile()
		print "-" * 40
