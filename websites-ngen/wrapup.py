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
from time import strftime, strptime
import pdb
from ConfigParser import SafeConfigParser
from sets import Set

MARKDOWN_COMMAND = "perl Markdown.pl" # could also be "perl Markdown.pl"
TITLE_REGEX = re.compile(r"<h1.*>(.*)</h1>")
LANGUAGE_EXTRACTOR = re.compile(r"^.*\.(.{2,5})\..*$") # anything from “.es.” to “.zh-TW.”

class MissingOptionError(Exception):
	def __init__(self, option, section):
		self.option = option
		self.section = section

	def __str__(self):
		return "Must define a '%s' in section %s" % (self.option, self.section)

class MarkdownWrapper(object):
	"""
	Important attributes:
	
		.sourceFilename			--	the file containing markdown text
		
		.destFilename			--	the output html file name
		
		.doctype				--	self explanatory
		
		.inHeadInherit			--	An inherited	string from inhead.inherit
									…to be included inside the head element
		
		.inHeadInclude			--	An uninherited	string from inhead.include
									…to be included right after .inHeadInherit
		
		.cssInherit				--	An inherited	string from css.inherit
									…to be included inside the head element
		
		.cssInclude				--	An uninherited	string from css.include
									…to be included right after .cssInherit
		
		.beforeContentInherit	--	An inherited	string from beforecontent.inherit.
									…to be included right before the content div
		
		.beforeContentInclude	--	An uninherited string from beforecontent.include.
									…to be included right before the content div
		
		.mainText				--	the text from .sourceFileName after it's been turned into HTML
		
		.afterMainTextInherit	--	An inherited	string from aftermaintext.inherit
									…included at the end of .mainText, but inside
									<div id='content'>
		
		.afterMainTextInclude	--	Like the above, but not inherited, and above it. I think.

		.afterContentInherit	--	stuff to include after the content div
			
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
		lm = LANGUAGE_EXTRACTOR.match(sourceFilename)
		if lm:
			self.lang = lm.group(1)
			

		self.inHeadInherit = ''
		self.inHeadInclude = ''
		
		self.cssInherit = ''
		self.cssInclude = ''
		
		self.langInclude = ''
		
		self.beforeContentInherit = ''
		self.beforeContentInclude = ''
		
		self.afterMainTextInherit = ''
		self.afterMainTextInclude = ''
		
		self.afterContentInherit  = ''
		self.afterContentInclude  = ''

		self.bodyAttributes = ''

		self.__loadIncludes()
		self.__loadOptions()
	
	def __loadOptions(self):
		
		def getBaseName(path):
			"""
				'index.html'			-> 'index'
				'index.es.markdown'		-> 'index.es'
				'/foo/bar/baz.es.txt'	-> 'baz.es'
			"""
			filename = os.path.split(path)[1]
			filenameWithoutExtension = os.path.splitext(filename)[0]
			return filenameWithoutExtension

		configFilename = getParallelFile(self.sourceFilename, 'options.ini')
		if not os.path.isfile(configFilename):
			return

		config = SafeConfigParser()
		config.read(configFilename)
		
		for section in config.sections():
			if section == getBaseName(self.sourceFilename):
				for key, value in config.items(section):
					if key == 'bodyattributes':
						self.bodyAttributes = ' ' + value
				

	def __loadIncludes(self):
		"""Loads .include and .inherit files into this object's attributes."""

		prefixes = "inhead css beforecontent aftermaintext aftercontent".split()
		inheritables = [i + ".inherit" for i in prefixes]
		includables =  [i + ".include" for i in prefixes]
		
		# this if cascade is ugly, ugly, ugly, ugly. Suggestions, please.
		def makeSearchers(baseList, ext):
			ret = []
			for base in baseList:
				s = r"^.*" + base + r".*" + ext + r"$"
				ret.append(re.compile(s))
			return tuple(ret)
				
		languagedPrefixes = prefixes[:]
		if self.lang != 'en-US':
			languagedPrefixes = [x + r'\.' + self.lang + r'\.' for x in languagedPrefixes]
		(INHEAD_INHERIT,
		 CSS_INHERIT,
		 BEFORECONTENT_INHERIT,
		 AFTERMAINTEXT_INHERIT,
		 AFTERCONTENT_INHERIT) = makeSearchers(languagedPrefixes, "inherit")
		(INHEAD_INCLUDE,
		 CSS_INCLUDE,
		 BEFORECONTENT_INCLUDE,
		 AFTERMAINTEXT_INCLUDE,
		 AFTERCONTENT_INCLUDE) = makeSearchers(languagedPrefixes, "include")

		for baseFilename in prefixes[:]:
			if self.lang != 'en-US':
				baseFilename += ".%s" % self.lang
			baseFilename += ".inherit"
			filename = getParallelFile(self.sourceFilename, baseFilename)
			filename = getParentedFile(filename)
			if not filename: continue
			if os.path.isfile(filename):
				contents = file(filename).read()
				if INHEAD_INHERIT.search(filename):
					self.inHeadInherit = contents
				if CSS_INHERIT.search(filename):
					self.cssInherit = contents
				if BEFORECONTENT_INHERIT.search(filename):
					self.beforeContentInherit = contents
				if AFTERMAINTEXT_INHERIT.search(filename):
					self.afterMainTextInherit = contents
				if AFTERCONTENT_INHERIT.search(filename):
					self.afterContentInherit = contents
		for baseFilename in prefixes:
			if self.lang != 'en-US':
				baseFilename += ".%s" % self.lang
			baseFilename += ".include"

			filename = getParallelFile(self.sourceFilename, baseFilename)
			# notice the complete lack of getParentedFile()
			if not filename: continue
			if os.path.isfile(filename):
				contents = file(filename).read()
				if INHEAD_INCLUDE.search(filename):
					self.inHeadInclude = contents
				if CSS_INCLUDE.search(filename):
					self.cssInclude = contents
				if BEFORECONTENT_INCLUDE.search(filename):
					self.beforeContentInclude = contents
				if AFTERMAINTEXT_INCLUDE.search(filename):
					self.afterMainTextInclude = contents
				if AFTERCONTENT_INCLUDE.search(filename):
					self.afterContentInclude = contents
		langFilename = getParallelFile(self.sourceFilename, "lang.include")
		if langFilename and os.path.isfile(langFilename):
			self.langInclude = file(langFilename).read()

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
		ret +=	"	<meta http-equiv='Content-type' content='text/html; charset=UTF-8'>\n"
		ret +=	self.inHeadInherit
		ret +=	self.inHeadInclude
		ret +=	'	<title>' + self.title + '</title>\n'
		ret +=	self.cssInherit
		ret +=	self.cssInclude
		ret += "</head>\n"
		ret += "<body"
		ret += self.bodyAttributes
		ret += ">\n"
		ret +=  self.langInclude
		ret +=	self.beforeContentInherit
		ret +=	self.beforeContentInclude
		ret += 	"<div id='content'>\n"
		ret +=	self.mainText
		ret +=	self.afterMainTextInclude
		ret +=	self.afterMainTextInherit
		ret +=	"</div>\n"
		ret +=	self.afterContentInclude
		ret +=	self.afterContentInherit
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
	
	return snipBom(ret)

def snipBom(s):
	BOM = '\xef\xbb\xbf'
	ret = ''
	bomBegin = s.find(BOM)
	if bomBegin > -1:
		ret = s[:bomBegin] + s[bomBegin+3:]
	else:
		return s
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
	
	Returns "" if no such file is found.
	"""
	while True:
		# okay, we need to abort out on two conditions:
		# 1) we found a valid path (return it)
		# 2) we've gone as far as we can, and there is no valid path (return "")
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

class NewsItem(object):
	def __init__(self):
		self.post = ''
		self.title = ''
		self.summary = ''
		self.tag = ''
		self.dateCreated = None
		self.dateIssued = None
		self.dateModified = None
		self.sections = Set()
	
	def __cmp__(self, other):
		return cmp(self.dateCreated, other.dateCreated)

def reverser(lhs, rhs):	# primarily to sort NewsItems newest-first
	return -cmp(lhs, rhs)
	

class NewsDispenser(object):
	def __init__(self, optionsFile):
		"""Make a very simple list of news posts. This is not final code."""
		config = SafeConfigParser({'sections': 'general'})
		config.read(os.path.join("news", "options.ini"))

		FILENAME_DATE_FORMAT = "on%Y-%m-%dat%H%M"
		USUAL_TIME_FORMAT = "%Y-%m-%dT%H:%M:%SZ"
		
		self.newsItems = []
		# strftime: format as string; strptime: parse string and get a struct_time
		# okay, now we want to go through the sections and associate each with their newspost.
		for section in config.sections(): # section is string
			ni = NewsItem()
			ni.dateCreated = time.strptime(section, FILENAME_DATE_FORMAT)
			#ni.dateModified = ni.dateIssued = ni.dateCreated #XXX
			for key, value in config.items(section):
				if key == 'title':
					ni.title = value

				if key == 'summary':
					ni.summary = value

				if key == 'tag suffix':
					ni.tag = 'tag:xiph.org,%s:%s' % \
							  (time.strftime('%Y-%m-%d', ni.dateCreated), value)

				if key == 'sections':
					ni.sections = Set(value.split())

				if key == 'issued':
					ni.dateIssued = time.strptime(value, USUAL_TIME_FORMAT)
					
				if key == 'modified':
					ni.dateModified = time.strptime(value, USUAL_TIME_FORMAT)
					
	
			# now for some error checking…
			if not ni.title:	raise MissingOptionError('Title', section)
			if not ni.summary:	raise MissingOptionError('Summary', section)

				
			if not ni.tag:		raise MissingOptionError('Tag Suffix', section)

			# …and some error covering…
			if not ni.dateModified:
				ni.dateModified = ni.dateCreated
			if not ni.dateIssued:
				ni.dateIssued = ni.dateCreated
			filename = os.path.join('news', section + '.txt')
			s = markdown(filename)
			ni.post = s
			self.newsItems.append(ni)
		self.newsItems.sort(reverser)
	
	def dump(self):
		return self.newsItems[:]
	
	def getItems(startDate, endDate):
		"""
			Return news items from
		"""

class NewsFormatter(object):
	def __init__(self, newsItems): pass
	def formatOne(self, newsItem): pass
	def formatAll(self): pass

class HTMLNewsFormatter(NewsFormatter):
	""" This is just one HTML news formatter of possibly many, really."""
	def __init__(self, newsItems, dateFormat="%A, %B %d, %Y"):
		self.newsItems = newsItems
		self.dateFormat = dateFormat
	
	def formatOne(self, newsItem):
		ret =  "<h1>" + newsItem.title + "</h1>\n"
		ret += newsItem.post[:]
		ret += "<p>Posted %s by %s</p>" %  \
			   (time.strftime(self.dateFormat, newsItem.date), newsItem.author)
		ret += "\n<hr>\n\n"

		return ret
	
	def formatAll(self):
		ret = ''
		for ni in self.newsItems:
			ret += self.formatOne(ni)
		return ret

class AtomNewsFormatter(NewsFormatter):

	# Tag URIs: http://www.taguri.org/ (used for atom:id)

	def __init__(self, newsItems):
		self.newsItems = newsItems
		self.dateFormat = "%Y-%m-%dT%H:%M:%SZ" # lie and say it's at UTC (...Z)
	
	def formatOne(self, newsItem, index=0):
		ret =  "<entry>\n"
		ret += "<title>" + newsItem.title + "</title>\n"
		if newsItem.dateCreated:
			ret +=	"<created>%s</created>\n" % strftime(self.dateFormat, newsItem.dateCreated)
		if newsItem.dateIssued:
			ret +=	"<issued>%s</issued>\n" % strftime(self.dateFormat, newsItem.dateIssued)
		if newsItem.dateModified:
			ret +=	"<modified>%s</modified>\n" % strftime(self.dateFormat, newsItem.dateModified)
		ret +=		"<id>%s</id>\n" % newsItem.tag
		ret +=		"<link rel='alternate' type='text/html' href='http://www.xiph.org/news/'/>\n"
		ret +=		"<content type='text/html' mode='escaped'><![CDATA["
		ret +=		newsItem.post
		ret +=		"]]></content>\n"
		ret +=		"</entry>"
		return ret
	def formatAll(self):
		ret = ''
		ret +=	"<?xml version='1.0' encoding='utf-8'?>\n" \
				"<feed version='0.3' xmlns='http://purl.org/atom/ns#' xml:lang='en-US'>\n" \
				"<title>Xiph.Org News</title>\n" \
				"<link rel='alternate' type='text/html' href='http://www.xiph.org/news/'/>\n" \
				"<modified>%s</modified>\n" % \
					strftime(self.dateFormat, self.newsItems[0].dateModified)
		ret +=	"<generator url='http://svn.xiph.org/websites-ngen/wrapup.py'>Wrapup</generator>\n" \
				"<copyright>© %s Xiph.Org Foundation</copyright>\n" % time.gmtime()[0] + \
				"<author><name>Xiph.Org Foundation</name></author>\n\n"

		for index, newsItem in enumerate(self.newsItems):
			ret += self.formatOne(newsItem, index) + "\n\n"
		
		ret += "</feed>"
		return ret

def doSite():
	for dirpath, dirnames, filenames in os.walk('xiph.org'):
		for filename in filter(lambda x: x.endswith(".markdown"), filenames):
			srcPath = os.path.join(dirpath, filename)
			destFile = os.path.splitext(filename)[0] + ".html"
			destPath = getParallelFile(srcPath, destFile)
			print srcPath
			mw = MarkdownWrapper(srcPath, destPath)
			mw.renderToFile()
		if ".svn" in dirnames:
			dirnames.remove('.svn') # don't transform things in .svn directories

def doNews():
	nd = NewsDispenser(os.path.join('news', 'options.ini'))
	#print HTMLNewsFormatter(nd.dump()).formatAll()
	s = AtomNewsFormatter(nd.dump()).formatAll()
	fh = open(os.path.join('xiph.org', 'feeds', 'atom.xml'), 'w')
	fh.write(s)
	fh.close()

if __name__ == '__main__':
	#doNews()
	doSite()
