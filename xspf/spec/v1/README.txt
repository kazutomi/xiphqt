This directory contains source files for creating the version 1 flavor of the XSPF spec.

The important files are rfc2629.xslt and xspf2629.xml.  These files
are combined to make an HTML version of the spec as follows:

xsltproc rfc2629.xslt xspf2629.xml > xspf-v1.html

rfc2629.xslt is a modified version of Julian F. Reschke's XSLT
transformation from RFC2629 XML format to HTML.  The original,
unmodified, version is in rfc2629.xslt-orig.

xspf2629.xml is source for the XSPF spec, formatted per RFC 2629.
Note that Julian's XSLT does not support all features of RFC 2629, and
also that I have slightly modified Julian's XSLT to support new
features, so xspf2629.xml is tightly coupled with the XSLT file here.

The specification source document, HTML document, and modifications to
rfc2629.xslt are the work of Lucas Gonze <lucas@gonze.com>.
