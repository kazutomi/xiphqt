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



## Accessing our Subversion Repository With TortoiseSVN
<img src='1-checkout.png' class='tortoiseSvnScreenie'
     alt='Windows Explorer’s right-click menu with “Checkout...” selected'>
 
[TortoiseSVN][] is a Windows Explorer shell extension that will let you do Subversion
things from within Windows Explorer. Once you have it installed, create a folder where
you can group your checkouts together; we’ll use D:\Projects in these examples.

Inside of D:\Projects, right-click somewhere in the blank space and select the “Checkout...”
option from the menu.

<div style='clear: both;'>&nbsp;</div>

<img src='2-url.png' class='tortoiseSvnScreenie'
	 alt=''>

Now, type in the URL of the section of the repository that you want. Not sure what you want
or where it is? Have a look at <http://svn.xiph.org/> in this Web browser. If you’re not looking
for Icecast-specific things, what you want will probably be in a subdirectory of [/trunk/][].

<div style='clear: both;'>&nbsp;</div>

<img src='3-directory.png' class='tortoiseSvnScreenie'
     alt=''>

Now that you’ve found something…
<div style='clear: both;'>&nbsp;</div>
<div style='clear: both;'>&nbsp;</div>
## Accessing Our Subversion Repository With Commit Access

Briefly:

<div><code>$ svn co svn+ssh://svn.xiph.org/svn/xiph/</code></div>

For reduced hassle, charge up an SSH agent.

At length:

...


### …for Developers With Commit Access
One of the easier ways to manage [TortoiseSVN][].  [PuTTY][], [PuTTYgen][], and [Pageant][]. Generate a new SSH2 DSA
key, set its password, and save it somewhere safe.


	[Subversion]:	http://subversion.tigris.org/
	[TortoiseSVN]:	http://tortoisesvn.tigris.org/
	[/trunk/]:		http://svn.xiph.org/trunk/
	[PuTTY]:		http://www.chiark.greenend.org.uk/~sgtatham/putty/
	[PuTTYgen]:		http://
	[Pageant]:		http://

