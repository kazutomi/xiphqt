# -*- coding: UTF-8 -*-
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

import os, re, sys, glob, os.path, time, datetime 
import pdb
from ConfigParser import SafeConfigParser
MARKDOWN_COMMAND = "Markdown.pl" # could also be "perl Markdown.pl"
TITLE_REGEX = re.compile(r"<h1.*>(.*)</h1>")
class MarkdownWrapper(object):
	"""
	Important attributes:
	
		.sourceFilename		-- the file containing markdown text
		.destFilename		-- the output html file name
		.doctype			-- self explanatory
		.inHead				-- a list of strings to be included inside the head element
		.beforeContent		-- stuff to include before the content div
		.mainText			-- the text from .sourceFileName after it's been turned into HTML
		.afterMainText		-- comes just after .mainText, but inside the content div
		.afterContent		-- stuff to include after the content div
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
		
		self.mainText = markdown(sourceFilename)

		self.title = "Untitled Document"
		match = TITLE_REGEX.match(self.mainText)
		if match:									#TODO: needs decent error handling
			self.title = "Xiph.Org: " + match.group(1)
		
		self.lang = 'en-US'

		self.afterMainText = ''
		
		self.inHead = ''
		
		self.beforeContent = ''
		self.afterContent  = ''
		

		# Now we need to, generally, load in data to attributes based on file name.
		# In particular

		# 	inhead.include			# 	.inHead
		#	beforecontent.include	#	.beforeContent
		#	maintext.markdown		#	.mainText		# NOTE: not in includerList
		#	aftermaintext.include	#	.afterMainText
		#	aftercontent.include	#	.afterContent
		
		includerList = "inhead beforecontent aftermaintext aftercontent".split()
		includerList = [ i + ".include" for i in includerList]
		
		# this if cascade is ugly, ugly, ugly, ugly. Suggestions, please.
		
		for baseFilename in includerList:
			filename = getParallelFile(self.sourceFilename, baseFilename)
#			if filename.endswith('aftermaintext.include'): pdb.runcall(getParentedFile, filename)
			filename = getParentedFile(filename)
			if not filename: continue
			
			if os.path.isfile(filename):
				contents = file(filename).read()
				if filename.endswith('inhead.include'):			self.inHead			= contents
				if filename.endswith('beforecontent.include'):	self.beforeContent	= contents
				if filename.endswith('aftermaintext.include'):	self.afterMainText	= contents
				if filename.endswith('aftercontent.include'):	self.afterContent 	= contents

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
		ret += "<meta http-equiv='Content-type' content='text/html; charset=UTF-8'>\n"
		ret += self.inHead
		ret += '<title>' + self.title + '</title>\n'
		ret += "</head>\n"
		ret += "<body>\n"
		ret += self.beforeContent
		ret += "<div id='content'>\n"
		ret += self.mainText
		ret += self.afterMainText
		ret += "</div>\n"
		ret += self.afterContent
		ret += "</body>\n"
		ret += "</html>\n"
		return ret

def markdown(filename):
	"""Returns an HTML snippet from a filename in Markdown format."""
	ret = ""
	fh = os.popen(MARKDOWN_COMMAND + ' ' + filename)
	try:
		ret = fh.read()
	finally:
		fh.close()
	return ret

def getParallelFile(where, what):
	"""
		given f("/home/foo.txt", "bar.html"), returns "/home/bar.html".
	"""
	return os.path.join(os.path.split(where)[0], what)

def hasTwoDirseps(path):
	firstIndex = path.find(os.sep)
	if firstIndex == -1: return False

	secondIndex = path.find(os.sep, firstIndex+1)
	if secondIndex == -1: return False

	return True
def getParentedFile(filename):
	r"""
	given a path to a file, say, /meals/breakfast/sausage/index.html, tries to return
	the first valid file of:
		/meals/breakfast/sausage/index.html
		/meals/breakfast/index.html
		/meals/index.html
		/index.html
	Or given .\meals\breakfast\eggs\index.html:
		.\meals\breakfast\eggs\index.html
		.\meals\breakfast\index.html
		.\meals\index.html
		.\index.html
	Or even given cheeses\france\brie.html:
		cheeses\france\brie.html
		cheeses\brie.html
		brie.html	

	"""
	while True:
		# okay, we need to abort out on two conditions:
		# 1) we found a valid path
		# 2) we've gone as far as we can, and there is no valid path
		if os.path.isfile(filename): return filename

		# There is no parented file if there's only one separator in the path
		if filename.find(os.sep) == -1:
			return ""


		# if the full string has two directory separators in it or more, then
		# there are still intermediate directories to back up through.
		# We need to chop out the thing just before the last dirsep,
		# but end just before the penultimate dirsep.
		if hasTwoDirseps(filename):
			indexOfLastDirsep = filename.rfind(os.sep)
			indexOfPenultimateDirsep = filename.rfind(os.sep, 0, indexOfLastDirsep)
			filename = filename[:indexOfPenultimateDirsep] + filename[indexOfLastDirsep:]
			if os.path.isfile(filename): return filename
		elif filename.find(os.sep) > -1:
			# If it has only one dirsep, then it's one of the following forms (modulo / vs. \)
			#	* /foo.html
			#	* ./foo.html
			if os.path.isfile(filename): return filename
			else: return ""
		else:
			if os.path.isfile(filename): return filename
			else: return ""

class NewsDispenser:
	def __init__(self, optionsFile, printableDateFormat="%A, %B %d, %Y"):
		"""Make a very simple list of news posts. This is not final code."""
		config = SafeConfigParser({'sections': 'general'})
		config.read(os.path.join("news", "options.ini"))

		self.printableDateFormat = printableDateFormat
		FILENAME_DATE_FORMAT = "on%Y-%m-%dat%H%M"
		
		#                        Monday, January 1, 2100
		self.datedAndOptionedPostList = [] # list of (date, options, post) tuples
		
		# okay, now we want to go through the sections and associate each with their newspost.
		for section in config.sections(): # section is string
			l = []
			l.append(time.strptime(section, FILENAME_DATE_FORMAT))
			o = {}
			for key, value in config.items(section):
				if key == 'author': 
					o['author'] = value
				if key == 'sections':
					o['sections'] = value.split()
			l.append(o)
			filename = os.path.join('news', section + '.txt')
			s = markdown(filename)
			l.append(s)
			self.datedAndOptionedPostList.append(tuple(l))
	
	def prettifyPost(self, post):
		"""
			Takes in a (date, options, post) tuple and make it look like a real HTML snippet.
			Returns said snippet.
		"""
		(date, options, text) = post
		ret = text[:]
		ret += "<p>Posted %s by %s</p>" %  \
			   (time.strftime(self.printableDateFormat, date), options['author'])
		ret += "\n<hr>\n"

		return ret
		
	def dump(self):
		"""You will rot in the Hot Place if you modify the contents of this return value."""
		return self.datedAndOptionedPostList
	def dumpPretty(self):
		ret = ""
		for post in self.datedAndOptionedPostList:
			ret += self.prettifyPost(post)
		return ret

if __name__ == '__main__':
	n = NewsDispenser(os.path.join('news', 'options.ini'))
	print n.dumpPretty()
	for dirpath, dirnames, filenames in os.walk('xiph.org'):
		# bug if more than one .markdown file in a directory
		for filename in filter(lambda x: x.endswith(".markdown"), filenames):
			srcPath = os.path.join(dirpath, filename)
			print srcPath
			destPath = getParallelFile(srcPath, "index.html")

			mw = MarkdownWrapper(srcPath, destPath)
			mw.renderToFile()
		print "-" * 40
		if ".svn" in dirnames:
			dirnames.remove('.svn') # don't transform things in .svn directories
