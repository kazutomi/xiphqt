Accessing our Subversion Repository
===================================

If you just want to browse our [Subversion][] repository, head on over to <http://svn.xiph.org/>.
If you want to have a proper copy of our stuff on your own computer, you’ll probably find that
it’s easier to keep up with our development efforts with <kbd>svn co</kbd> than with
<kbd>wget -m -np</kbd> since you won’t get the Subversion metadata with the latter method.

The stock client is fine in both Unix-style OSs and on Windows, although you may want to consider
using [TortoiseSVN][] with [our Windows-specific instructions][win] if you’re using the latter.

## Getting Your Bearings

This section will make a lot more sense if you’re browsing <http://svn.xiph.org/> as you read this.
In particular, have a look at <http://svn.xiph.org/trunk/>; that’s where most of our stuff is.

If you want to build oggenc, you’ll need to check out the <kbd>vorbis-tools</kbd> directory,
along with the <kbd>ogg</kbd>, <kbd>vorbis</kbd>, and <kbd>ao</kbd> directories. To do this, open
up a terminal and type: 

<div><code>
	~ $ mkdir xiphstuff <br>
	~ $ cd xiphstuff <br>
	~/xiphstuff $ svn <abbr title='check out'>co</abbr>
							http://svn.xiph.org/trunk/ogg/				\<br>
							http://svn.xiph.org/trunk/vorbis/			\<br>
							http://svn.xiph.org/trunk/vorbis-tools/		\<br>
							http://svn.xiph.org/trunk/ao/
</code></div>

If you’re using Windows, the above won’t work unless you remove the backslashes.

## If You Have Commit Access

Briefly:

<div><code>
	$ svn co	svn+ssh://svn.xiph.org/svn/xiph/ogg/ \<br>
				svn+ssh://svn.xiph.org/svn/xiph/vorbis/ \<br>
				svn+ssh://svn.xiph.org/svn/xiph/vorbis-tools/ \<br>
				svn+ssh://svn.xiph.org/svn/xiph/ao/
</code></div>

Note the changed protocol (no, “ssh+svn” won’t work) and the addition of the
<kbd>/svn/xiph/</kbd> part.

For reduced hassle, charge up an SSH agent.

	[Subversion]:	http://subversion.tigris.org/
	[TortoiseSVN]:	http://tortoisesvn.tigris.org/
	[/trunk/]:		http://svn.xiph.org/trunk/
	[win]:			./windows/
	[PuTTY]:		http://www.chiark.greenend.org.uk/~sgtatham/putty/
	[ogg]:			http://svn.xiph.org/trunk/ogg/
	[vorbis]:		http://svn.xiph.org/trunk/vorbis/
	[ao]:			http://svn.xiph.org/trunk/ao/
	[vorbis-tools]:	http://svn.xiph.org/trunk/vorbis-tools/
