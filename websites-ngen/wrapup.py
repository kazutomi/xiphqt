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
import profile

MARKDOWN_COMMAND = "perl Markdown.pl --htmltags"
TITLE_REGEX = re.compile(r"<h1.*>(.*)</h1>")
LANGUAGE_EXTRACTOR = re.compile(r"^.*\.(.{2,5})\..*$") # anything from “.es.” to “.zh-TW.”
USUAL_TIME_FORMAT = "%Y-%m-%dT%H:%M:%SZ" # W3C Date-Time format, so I'm told

####################################################################################################
## Markdown Stuff
##

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
		self.doctype = '<!DOCTYPE HTML PUBLIC '  \
					   '"-//W3C//DTD HTML 4.01//EN" '  \
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
					if key == 'title':
						self.title = value
				

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

###################################################################################################
## News Item stuff
##

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
		self.url = ''
	
	def __str__(self):
		return '<NewsItem object created at %s entitled "%s">' % \
				(strftime(USUAL_TIME_FORMAT, self.dateCreated), self.title)
	def __repr__(self):
		return str(self)
	def __getTagSuffix(self):
		return self.tag.split(':')[-1]
	tagSuffix = property(__getTagSuffix)
	def __cmp__(self, other): return NewsItem.compareByDateCreated(self, other)
	def compareByDateCreated(self, other):	return cmp(self.dateCreated,  other.dateCreated)
	def compareByDateIssued(self, other):	return cmp(self.dateIssued,   other.dateIssued)
	def compareByDateModified(self, other):	return cmp(self.dateModified, other.dateModified)

def reverser(lhs, rhs, comparator=cmp):	# primarily to sort NewsItems newest-first
	return -comparator(lhs, rhs)

class NewsDispenser(object):
	def __init__(self, optionsFile):
		config = SafeConfigParser({'sections': 'general'})
		config.read(os.path.join("news", "options.ini"))

		
		self.newsItems = []
		# strftime: format as string; strptime: parse string and get a struct_time
		# okay, now we want to go through the sections and associate each with their newspost.
		for section in config.sections(): # section is string
			ni = NewsItem()
			ni.tag = 'tag:xiph.org,' + section
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

				if key == 'created':
					ni.dateCreated = time.strptime(value, USUAL_TIME_FORMAT)
					
				if key == 'issued':
					ni.dateIssued = time.strptime(value, USUAL_TIME_FORMAT)
					
				if key == 'modified':
					ni.dateModified = time.strptime(value, USUAL_TIME_FORMAT)
					
	
			# now for some error checking…
			if not ni.title:	raise MissingOptionError('Title', section)
			if not ni.summary:	raise MissingOptionError('Summary', section)
			if len(ni.sections) == 0:
				raise MissingOptionError('Sections', section)

			# …and some error covering…
			# "date created" absolutely must be there. Everything else can be generated from that.
			if not ni.dateCreated:
				raise MissingOptionError('Created', section)
			if not ni.dateModified:
				ni.dateModified = ni.dateCreated
			if not ni.dateIssued:
				ni.dateIssued = ni.dateCreated
			
			# sections look like [2004-04-21:speex115]
			tagBits = re.compile(
				r"^(?P<year>\d\d\d\d)-(?P<month>\d\d)-(?P<day>\d\d):(?P<tag>\w*)$")
			m = tagBits.match(section)
			if not m:
				raise "the tag bits regex didn't match"
			
			# make sure the two required dates match
			ymdFromDateCreated = list(ni.dateCreated[:3])
			ymdFromRegex = [int(m.group('year')), int(m.group('month')), int(m.group('day'))]
			if ymdFromDateCreated != ymdFromRegex:
				raise	"The date in section %s does not match its Created key %s, " \
						"when was it _really_ created? " % \
						(section, "%s-%s-%s" % ni.dateCreated[:3])

			ni.url = "http://www.xiph.org/news/%s/%s/%s/%s/" % (
					 m.group('year'),
					 m.group('month'),
					 m.group('day'),
					 m.group('tag'))

			filename = os.path.join('news',
			                        m.group('year'),
									m.group('month'),
									m.group('day'),
									m.group('tag') + '.markdown')
			s = markdown(filename)
			ni.post = s
			self.newsItems.append(ni)
		self.newsItems.sort(reverser)
	
	def items(self):
		return self.newsItems
	
	def __itemsByTimespan(self, unitCode):
		"""
			unitCode == 1? By year.
			unitCode == 2? By month.
			unitCode == 3? By day.
		"""
		allDates = [] # list of `unitCode`-tuples in (year[, month[, day]]) format
		ret = []
		for date in [item.dateCreated[:unitCode] for item in self.newsItems]:
			if date not in allDates:
				allDates.append(date)
		
		for date in allDates:
			itemsOfTheTimespan = [item for item in self.newsItems \
							 if item.dateCreated[:unitCode] == date]
			ret.append(itemsOfTheTimespan)

		return ret
	def itemsByDay(self):
		"Returns a list of lists of items all with the same day."
		return self.__itemsByTimespan(3)
	
	def itemsByMonth(self):
		return self.__itemsByTimespan(2)
	
	def itemsByYear(self):
		return self.__itemsByTimespan(1)

		
	

###################################################################################################
## News Formatters
##
	
class NewsFormatter(object):
	def __init__(self, dispenser):
		self.dispenser = dispenser
	
	def format(self, numberOfItems=0):
		"""
			Returns all news items as a formatted string.
			Override this.
		"""
	def formatByIndex(self, index): pass
	def formatOne(self, newsItem): pass

class HTMLNewsFormatter(NewsFormatter):
	def __init__(self, newsItems, dateFormat="%A, %B %d, %Y"):
		NewsFormatter.__init__(self, newsItems)
		self.newsItems = newsItems
		self.dateFormat = dateFormat
	
	def formatForDay(self, year, month, day):
		"return a formatted string for the day in question."
		loloni = self.dispenser.itemsByDay() # list of lists of NewsItems
		ret = ''
		
		matchingDayGroups = [x for x in loloni if x[0].dateCreated[:3] == (year, month, day)]
		for dayGroup in matchingDayGroups:
			ret +=	'<h1>News for %s</h1>\n' % strftime(self.dateFormat, dayGroup[0].dateIssued)
			for item in dayGroup:
				ret += self.formatOne(item, headingLevel=2)

		return ret
	
	def formatForMonth(self, year, month):
		loloni = self.dispenser.itemsByMonth()
		ret = ''

		for monthGroup in [x for x in loloni if x[0].dateCreated[:2] == (year, month)]:
			ret += strftime("<h1>News for %B %Y</h1>\n", monthGroup[0].dateIssued)
			for item in monthGroup:
				ret += self.formatOne(item, headingLevel=2)
		return ret

	def formatForYear(self, year):
		loloni = self.dispenser.itemsByYear()
		ret = ''
		for yearGroup in [x for x in loloni if x[0].dateCreated[:1] == (year,)]:
			ret += strftime("<h1>News for %Y</h1>\n", yearGroup[0].dateIssued)
			for item in yearGroup:
				ret += self.formatOne(item, headingLevel=2)
		return ret
		
		
	def formatOne(self, newsItem, headingLevel=1):
		reg = re.compile(r'^http://www.xiph.org(.*)$')
		m = reg.match(newsItem.url)
		url = m.groups(1)

		ret =  "<div class='newsEntry'>\n"
		ret += " <h%s>%s</h%s>\n" % (headingLevel, newsItem.title, headingLevel)
		ret +=  newsItem.post[:]
		ret += " <div class='postInfo'>Posted %s (<a href='%s'>permalink</a>)</div>\n" % \
			    (time.strftime(self.dateFormat, newsItem.dateIssued), url)
		ret += "</div>\n\n"

		return ret
	
	def format(self, howMany=0):
		"""Formats all in the series if howMany is 0; formats and returns that many otherwise."""
		ret = ''
		items = self.dispenser.items()
		# lame way of coding, but what the hey
		if howMany == 0:
			for ni in items:
				ret += self.formatOne(ni)
			return ret
		else:
			for ni in items[:min(howMany, len(items))]:
				ret += self.formatOne(ni)
			return ret

class AtomNewsFormatter(NewsFormatter):
	def __init__(self, newsItems):
		NewsFormatter.__init__(self, newsItems)
		self.dateFormat = "%Y-%m-%dT%H:%M:%SZ" # lie and say it's at UTC (...Z)
	
	def formatOne(self, newsItem):
		def formatDate(date):
			return strftime(self.dateFormat, date)

		ret =  "<entry>\n"
		ret += " <title>" + newsItem.title + "</title>\n"
		ret += " <created>%s</created>\n"	%	formatDate(newsItem.dateCreated)
		ret += " <issued>%s</issued>\n"		%	formatDate(newsItem.dateIssued)
		ret += " <modified>%s</modified>\n"	%	formatDate(newsItem.dateModified)
		ret += " <id>%s</id>\n"				%	newsItem.tag
		ret += " <link rel='alternate' type='text/html' href='%s'/>\n" % newsItem.url
		ret += " <content type='text/html' mode='escaped'><![CDATA["
		ret += newsItem.post
		ret += "]]></content>\n"
		ret += "</entry>"
		return ret
	
	def format(self):
		ret = ''
		ret +=	"<?xml version='1.0' encoding='utf-8'?>\n" \
				"<feed version='0.3' xmlns='http://purl.org/atom/ns#' xml:lang='en-US'>\n" \
				"<title>Xiph.Org News</title>\n" \
				"<link rel='alternate' type='text/html' href='http://www.xiph.org/news/'/>\n" \
				"<modified>%s</modified>\n" % \
					strftime(self.dateFormat, self.dispenser.items()[0].dateModified)
		ret +=	"<generator url='http://svn.xiph.org/websites-ngen/wrapup.py'>" \
				"Wrapup" \
				"</generator>\n" \
				"<copyright>© %s Xiph.Org Foundation</copyright>\n" % time.gmtime()[0] + \
				"<author><name>Xiph.Org Foundation</name></author>\n\n"

		for index, newsItem in enumerate(self.dispenser.items()):
			ret += self.formatOne(newsItem) + "\n\n"
		
		ret += "</feed>"
		return ret

###################################################################################################
## Miscellaneous Functions and Exception Classes
##

def passPrint(x):
	print x
	return x

class MissingOptionError(Exception):
	def __init__(self, option, section):
		self.option = option
		self.section = section

	def __str__(self):
		return "Must define a '%s' in section %s" % (self.option, self.section)


def markdown(filename):
	"""Returns an HTML snippet from a filename in Markdown format."""
	ret = ""

	if not os.path.isfile(filename):
		raise RuntimeError("Can not markdown non-existent file " + filename)

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
		

def hasTwoDirseps(path):
	firstIndex = path.find(os.sep)
	if firstIndex == -1: return False

	secondIndex = path.find(os.sep, firstIndex+1)
	if secondIndex == -1: return False

	return True


def getParallelFile(where, what):
	"""
		given f("/home/foo.txt", "bar.html"), returns "/home/bar.html".
	"""
	return os.path.join(os.path.split(where)[0], what)


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


def getBaseName(path):
	"""
		'index.html'			-> 'index'
		'index.es.markdown'		-> 'index.es'
		'/foo/bar/baz.es.txt'	-> 'baz.es'
	"""
	filename = os.path.split(path)[1]
	filenameWithoutExtension = os.path.splitext(filename)[0]
	return filenameWithoutExtension

def padto2(s):
	if len(s) > 1:
		return s
	else:
		return "0" + s


###################################################################################################
## Driver Functions
##

def doSite():
	for dirpath, dirnames, filenames in os.walk('xiph.org'):
		for filename in [x for x in filenames if x.endswith('.markdown')]:
			srcPath = os.path.join(dirpath, filename)
			destFile = os.path.splitext(filename)[0] + ".html"
			destPath = getParallelFile(srcPath, destFile)
			print srcPath
			mw = MarkdownWrapper(srcPath, destPath)
			mw.renderToFile()
		if ".svn" in dirnames:
			dirnames.remove('.svn') # don't transform things in .svn directories

def doNews():
	def doNewsInAtom(newsDispenser):
		s = AtomNewsFormatter(newsDispenser).format()
		fh = open(os.path.join('xiph.org', 'feeds', 'atom.xml'), 'w')
		fh.write(s)
		fh.close()
	def doNewsInHTML(newsDispenser):
		def entrywise(formatter):
			for item in formatter.dispenser.items():
				s = formatter.formatOne(item)
				(year, month, day) = [padto2(str(x)) for x in item.dateCreated[:3]]
				destPath = os.path.join('xiph.org', 'news', year, month, day, item.tagSuffix)
				try: os.makedirs(destPath)
				except OSError: pass # the directory was already there
				fh = open(os.path.join(destPath, 'index.markdown'), 'w')
				try:
					fh.write(s)
				finally:
					fh.close()
		

		def daily(formatter):
			for itemGroup in formatter.dispenser.itemsByDay():
				(year, month, day) = [padto2(str(x)) for x in itemGroup[0].dateCreated[:3]]
				destPath = os.path.join('xiph.org', 'news', year, month, day, 'index.markdown')
				fh = open(destPath, 'w')
				try:
					fh.write(formatter.formatForDay(int(year), int(month), int(day)))
				finally:
					fh.close()


		def monthly(formatter):
			for itemGroup in formatter.dispenser.itemsByMonth():
				(year, month) = [padto2(str(x)) for x in itemGroup[0].dateCreated[:2]]
				destPath = os.path.join('xiph.org', 'news', year, month, 'index.markdown')
				fh = open(destPath, 'w')
				try:
					fh.write(formatter.formatForMonth(int(year), int(month)))
				finally:
					fh.close()
					
		def yearly(formatter):
			for itemGroup in formatter.dispenser.itemsByYear():
				(year,) = [padto2(str(x)) for x in itemGroup[0].dateCreated[:1]]
				destPath = os.path.join('xiph.org', 'news', year, 'index.markdown')
				fh = open(destPath, 'w')
				try:
					fh.write(formatter.formatForYear(int(year)))
				finally:
					fh.close()

		def toplevel(formatter):
			howManyOnFrontPage = len(formatter.dispenser.items())
			if howManyOnFrontPage > 5:
				howManyOnFrontPage = 5
			for item in formatter.dispenser.items()[:howManyOnFrontPage]:
				destPath = os.path.join('xiph.org', 'news', 'index.markdown')
				fh = open(destPath, 'w')
				try:
					fh.write(formatter.format(howManyOnFrontPage))
				finally:
					fh.close()
				

		formatter = HTMLNewsFormatter(newsDispenser)
		
		for f in [entrywise, daily, monthly, yearly, toplevel]:
			f(formatter)

	sys.stdout.write('Preparing news posts...')
	nd = NewsDispenser(os.path.join('news', 'options.ini'))
	doNewsInAtom(nd)
	doNewsInHTML(nd)
	sys.stdout.write('done!\n')
		
def main():
	doNews()
	doSite()

if __name__ == '__main__':
	if len(sys.argv) > 1 and sys.argv[1] == '--profile':
		profile.run('main()')
	else:
		main()
