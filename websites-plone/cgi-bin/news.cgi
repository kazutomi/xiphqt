#!/usr/bin/python
"""
Translate a collection of news rst files into an XHTML fragment
for inclusion on a webpage.
"""

__docformat__ = 'reStructuredText'

NEWSROOT = "/var/www/www.xiph.org/news"

import os
import sys
import re
from glob import glob
from docutils.core import publish_string
from docutils import nodes, utils, writers, languages

class Writer(writers.Writer):
    supported = ('xhtml',)
    output = None

    def __init__(self):
        writers.Writer.__init__(self)

    def translate(self):
        visitor = XHTMLTranslator(self.document)
        self.document.walkabout(visitor)
        self.output = visitor.astext()

class XHTMLTranslator(nodes.NodeVisitor):

    def __init__(self, document):
        nodes.NodeVisitor.__init__(self, document)
	self.body = []
        
    def astext(self):
        return ''.join(self.body)

    def encode(self, text):
        text = text.replace('&', "&amp;")
	text = text.replace('<', "&lt;")
	text = text.replace('"', "&quot;")
	text = text.replace('>', "&gt;")
	return text

    def atval(self, text, whitespace=re.compile('[\n\r\t\v\f]')):
        return self.encode(whitespace.sub(' ', text))

    def visit_Text(self, node):
        self.body.append(self.encode(node.astext()))

    def depart_Text(self, node):
        pass

    def visit_document(self, node):
        pass

    def depart_document(self, node):
        pass

    def visit_paragraph(self, node):
        self.body.append('<p>')

    def depart_paragraph(self, node):
        self.body.append('</p>')

    def visit_reference(self, node):
        if node.has_key('refuri'):
            href = node['refuri']
        self.body.append('<a href="' + href + '">')

    def depart_reference(self, node):
        self.body.append('</a>')

    def visit_target(self, node):
        pass

    def depart_target(self, node):
        pass

    def visit_title(self, node):
        self.body.append('<span class="news-title">')

    def depart_title(self, node):
        self.body.append('</span>')

date_re = re.compile("^(\d\d\d\d)(\d\d)(\d\d)-(\d\d)(\d\d)$")
def parse_date(date):
    m = date_re.match(date)
    if not m:
        raise StandardError, "Not a valid date string"

    return "%s-%s-%s %s:%s" % (m.group(1), m.group(2), m.group(3),
                               m.group(4), m.group(5))

if __name__ == '__main__':
    print "Content-Type: text/html"
    print
    files = glob(NEWSROOT + os.sep + "*.rst")
    files.sort()
    files.reverse()
    for fn in files:
        f = open(fn)
        date = parse_date(os.path.splitext(os.path.basename(fn))[0])
        print '<span class="news-date">' + date + '</a>'
        print publish_string(source=f.read(),
                             writer=Writer())
        print
        f.close()

