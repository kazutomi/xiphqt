# Accessing our Subversion Repository

If you just want to browse our [Subversion][] repository, head on over to <http://svn.xiph.org/>.
If you want to have a proper copy on disk, you’ll need to download a Subversion client.
The stock client is fine in both Unix-style OSs and on Windows, although you may want to consider
using [TortoiseSVN][] if you’re using the latter.

An example initial checkout:

<div><code>
	$ mkdir xiphstuff <br>
	$ cd xiphstuff <br>
	$ svn co	http://svn.xiph.org/trunk/ogg/
				http://svn.xiph.org/trunk/vorbis/
				http://svn.xiph.org/trunk/vorbis-tools/
</code></div>


	[Subversion]:	http://subversion.tigris.org/
	[TortoiseSVN]:	http://tortoisesvn.tigris.org/


## Accessing Our Subversion Repository With Commit Access

Briefly:

<div><code>$ svn co svn+ssh://svn.xiph.org/svn/xiph/</code></div>

For reduced hassle, charge up an SSH agent.

At length:

...


### …for Developers With Commit Access
One of the easier ways to manage [TortoiseSVN][].  [PuTTY][], [PuTTYgen][], and [Pageant][]. Generate a new SSH2 DSA
key, set its password, and save it somewhere safe.


	[PuTTY]:		http://www.chiark.greenend.org.uk/~sgtatham/putty/
	[PuTTYgen]:		http://
	[Pageant]:		http://
