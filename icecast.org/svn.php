<? include "common/header.php"; ?>
<h2>Subversion</h2>
<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Access to Subversion Repository</h3>
<p>Xiph.org has switched from CVS to using Subversion.  More information regarding subversion can be found <a href="http://subversion.tigris.org/">here</a>.  Before following these instructions make sure that you have a copy of Subversion installed.  These instructions will assume that you are using a command-line version.</p>
<h3>1. Create a place to store the local repository</h3>
<pre>
$ mkdir $HOME/xiphrepository  # Obviously this name can be whatever you'd like..
                                this is an example.
$ cd $HOME/xiphrepository
</pre>
<h3>2. Checkout the main icecast trunk</h3>
<pre>
$ svn co http://svn.xiph.org/icecast/trunk/icecast/ icecast
A  icecast/debian
A  icecast/debian/icecast2.postinst
A  icecast/debian/icecast2.postrm
A  icecast/debian/icecast2.default
A  icecast/debian/control
....
Checked out revision 6618.  # Note your revision may be different
</pre>
<h3>3. Run autogen.sh in the main icecast root</h3>
Note: You must have libtool, autoconf, and automake to perform this step.
<pre>
$ cd $HOME/xiphrepository/icecast
$ ./autogen.sh
Checking for automake version
found automake-1.6
found aclocal-1.6
Generating configuration files for icecast, please wait....
aclocal-1.6  -I m4
autoheader
libtoolize --automake
automake-1.6 --add-missing
...
config.status: creating win32/Makefile
config.status: creating win32/res/Makefile
config.status: creating config.h
config.status: executing depfiles commands
$
</pre>
<h3>4. Verfiy everything by performing a make</h3>
<pre>
$ cd $HOME/xiphrepository/icecast
$ make
</pre>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br><br>
<? include "common/footer.php"; ?>


