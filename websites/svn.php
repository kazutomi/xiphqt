<? include "header.php"; ?>
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
<h3>3. Checkout the m4 module</h3>
<pre>
$ cd $HOME/xiphrepository/icecast
$ svn co http://svn.xiph.org/icecast/trunk/m4/ m4
A  m4/acx_pthread.m4
A  m4/shout.m4
A  m4/xiph_compiler.m4
A  m4/theora.m4
A  m4/vorbis.m4
A  m4/xiph_path_python.m4
A  m4/xiph_xml2.m4
A  m4/ac_config_libconfig_in.m4
A  m4/xiph_types.m4
A  m4/ogg.m4
A  m4/xiph_curl.m4
A  m4/ogg2.m4
A  m4/xiph_net.m4
Checked out revision 6618.
</pre>
<h3>4. Checkout all dependent icecast modules</h3>
<pre>
$ cd $HOME/xiphrepository/icecast/src
$ svn co http://svn.xiph.org/icecast/trunk/avl/ avl
A  avl/test.c
A  avl/avl.c
A  avl/TODO
A  avl/avl.dsp
A  avl/avl.h
A  avl/COPYING
A  avl/Makefile.am
A  avl/.cvsignore
A  avl/README
A  avl/BUILDING
Checked out revision 6618.
$ svn co http://svn.xiph.org/icecast/trunk/thread/ thread
A  thread/TODO
A  thread/COPYING
A  thread/thread.c
A  thread/Makefile.am
A  thread/thread.h
A  thread/BUILDING
A  thread/README
A  thread/.cvsignore
Checked out revision 6618.
$ svn co http://svn.xiph.org/icecast/trunk/httpp/ httpp
A  httpp/test.c
A  httpp/httpp.c
A  httpp/TODO
A  httpp/httpp.h
A  httpp/COPYING
A  httpp/Makefile.am
A  httpp/README
A  httpp/BUILDING
A  httpp/.cvsignore
Checked out revision 6618.
$ svn co http://svn.xiph.org/icecast/trunk/net/ net
A  net/sock.c
A  net/test_resolver.c
A  net/resolver.c
A  net/TODO
A  net/sock.h
A  net/resolver.h
A  net/COPYING
A  net/Makefile.am
A  net/README
A  net/BUILDING
A  net/.cvsignore
Checked out revision 6618.
$ svn co http://svn.xiph.org/icecast/trunk/log/ log
A  log/test.c
A  log/log.c
A  log/log.h
A  log/Makefile.am
A  log/.cvsignore
Checked out revision 6618.
$ svn co http://svn.xiph.org/icecast/trunk/timing/ timing
A  timing/TODO
A  timing/timing.c
A  timing/COPYING
A  timing/Makefile.am
A  timing/.cvsignore
A  timing/README
A  timing/BUILDING
A  timing/timing.h
Checked out revision 6618.
</pre>
<h3>5. Run autogen.sh in the main icecast root</h3>
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
<h3>6. Verfiy everything by performing a make</h3>
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
<? include "footer.php"; ?>


