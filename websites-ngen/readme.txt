About This Ball of Wax
======================

Requirements
------------

*	Python 2.3.2, to run wrapup.py (use earlier versions at your own risk)
*	Perl 5.6.1, to run Markdown (ActivePerl works fine)
*	Apache2, if you want to view your changes (and you probably do)

Trying It Out
-------------

Run wrapup.py right in the directory it's in. If nothing failed, you should see a new .html
file for every .markdown file.

Apache Setup
------------

I try to keep as much configuration stuff in the .htaccess file as I possibly can
(running apachectl is a pain), but there are a couple of things you need to have in the main
httpd.conf or wherever you store your apache setup:

	<?>
		# The following may be okay without Includes. You also might be able to chuck Indexes,
		# but I never do that.
		# FollowSymLinks is an absolute must for the non-English versions, since mod_rewrite
		# needs it as a security check or something.
		Options Indexes Includes FollowSymLinks

		AllowOverride All
	</?>
