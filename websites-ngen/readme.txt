About This Ball of Wax
======================

Requirements
------------

*	Python 2.3.2, to run wrapup.py (use earlier versions at your own risk)
*	Perl 5.6.1, to run Markdown (ActivePerl works fine)
*	Apache, if you want to view your changes

Trying It Out
-------------

Run wrapup.py right in the directory it's in. If nothing failed, you should see a new .html
file for every .markdown file.

Assumptions
-----------

All URLs are to be in the style of /foo/bar/, for reasons described on [Slash Forward][].
I may break this for content-negotiated language-specific variants (say, for /foo/bar/es for
"/foo/bar/ in Spanish"), but this is the way it's set up now.

	[Slash Forward]: http://www.alistapart.com/articles/slashforward/
