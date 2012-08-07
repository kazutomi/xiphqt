<h2>News</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div id="ices_release_2.0.2" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Ices Release 2.0.2</h3>
<p>After far a long time the icecast development team has released Ices version 2.0.2.
Ices is a source client for Icecast streaming media server. It takes audio from one
of the defined inputs and (re)encodes the audio for streaming to icecast for listeners
to pick up.</p>
<p>
<ul>
 <li>New features -
  <ul>
   <li> Support for RoarAudio input
  </ul>
  <br />
 <li>Bugfixes -
  <ul>
   <li> Updated Documentation(Closes: #1238)
   <li> Fixed double shout_close() (Closes: #720)
   <li> Reduced error to warning in case of duplicate serial number.
   <li> Fixed handling of <reconnectattempts> when set to zero (Closes: #735)
   <li> Did some code cleanup and hardening (Closes: #1795)
   <li> Enabled compiler warnings (Closes: #1796)
   <li> Added <retry-initial> (Closes: #994)
   <li> Updated handling of empty strings in config file (Closes: #1875)
   <li> Updated build system
  </ul>
  <br />
 </ul>
</p>
<p>Download Ices 2.0.2 from the <a href="ices.php">Ices page</a>.</p>

<div class="poster">
Posted Aug 6, 2012 by dm8tbr
</div>
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br />
<br />


<div id="release_2.4-beta" class="newscontent">
<h3>Icecast Release 2.4-beta</h3>
We are pleased to announce a beta release of Icecast. A summary of the changes is listed below
<br />
Please note that while Opus and WebM are considered production ready, there might be other problems in this release. We appreciate feedback and discussion on the icecast-dev mailing list!
<br />
<br />
<p>Downloads:<br />
<center>
<table border=0>
<tr><td>Source :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.4-beta.tar.gz">icecast-2.4-beta.tar.gz</a></td></tr>
</table>
</center>
<p>
<br />
<ul>
<li>New features -
<ul>
  <li> Support for Ogg Opus streams
  <li> Support for WebM streams
</ul><br />
<li>Bugfixes -
<ul>
  <li> Fixes for some race conditions
  <li> Allow (standard strftime(3)) %x codes in &lt;dump-file&gt;. Disabled for win32.
  <li> Dropped debian packaging directory as debian use their own.
</ul><br />
</ul>
<div class="poster">
Posted July 18, 2012 by dm8tbr
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br />
<br />

<div class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div id="release_2.3.3" class="newscontent">
<h3>Icecast Release 2.3.3</h3>
We are pleased to announce the next release of Icecast. A summary of the changes are listed below
<br />
<br />
<p>Downloads:<br />
<center>
<table border=0>
<tr><td>Source :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.3.tar.gz">icecast-2.3.3.tar.gz</a></td></tr>
<tr><td>Windows Setup :</td><td>Not yet available. Will be provided ASAP.<!--<a href="http://downloads.xiph.org/releases/icecast/icecast2_win32_2.3.3_setup.exe">icecast2_win32_2.3.3_setup.exe</a>--></td></tr>
</table>
</center>
<p>
<br />
<ul>
<li>Security -
We fixed 3 security issues.
<ul>
  <li>Improved HTTPS cipher handling and added support for chained certificates.
  <li>Allow the source password to be undefined. There was a corner case, where a default password would have taken effect. It would require the admin to remove the 'source-password' from the icecast config to take effect. Default configs ship with the password set, so this vulnerability doesn't trigger there.
  <li>Prevent error log injection of control characters by substituting non-alphanumeric characters with a '.' (CVE-2011-4612). Injection attempts can be identified via access.log, as that stores URL encoded requests. Investigation if further logging code needs to have sanitized output is ongoing.
</ul><br />
<li>Bugfixes -
This is mostly a bug-fix release to flush out the accumulated, mostly small, fixes.<br />
<ul>
  <li>On-demand relaying - Reject listeners while reconnecting. Fix stats for relays without mount section.
  <li>Prevent too frequent YP updates.
  <li>Only allow raw metadata updates from same IP as connected source (unless user is admin). This addresses broken client software that issues updates without being connected.
  <li>Minor memory leaks
  <li>XSPF file installation
  <li>Fix case of global listeners count becoming out of sync.
  <li>Setting an interval of 0 in mount should disable shoutcast metadata inserts.
</ul><br />
<li>Authentication
<ul>
  <li>Sources can now be authenticated via URL, like listeners.
Post info is "action=stream_auth&mount=/stream&ip=IP&server=SERVER&port=8000&user=fred&pass=pass "
As admin requests can come in for a stream (eg metadata update) these requests 
can be issued while stream is active. For these &admin=1 is added to the POST 
details. 
</ul><br />
<li>XSL update
<ul>
  <li>Automatically generate VCLT playlist like we do with M3U, the mountpoint extension is .vclt
</ul><br />
<li>Documentation updates.
</ul>
<div class="poster">
Posted June 11, 2012 by dm8tbr
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	
<br />
<br />
<div id="libshout_release_2.3.1" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>libShout Release 2.3.1</h3>
<p>Added support for Opus, contributed by Greg Maxwell.</p>

<p>Download libshout 2.3.1 from the <a href="/download.php#lib">Download page</a>.</p>

<div class="poster">
Posted May 25, 2012 by dm8tbr
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div id="libshout_release_2.3.0" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>libShout Release 2.3.0</h3>
<p>Added support for WebM streams, contributed by David Richards.</p>


<div class="poster">
Posted Feb 2, 2012 by dm8tbr
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>

<div id="release_2.3.2" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.3.2</h3>
We are pleased to announce the next release of Icecast. A summary of the changes are listed below
<br />
<br />
<p>Downloads:<br />
<center>
<table border=0>
<tr><td>Source :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.2.tar.gz">icecast-2.3.2.tar.gz</a></td></tr>
<tr><td>SRPM :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.2-0.src.rpm">icecast-2.3.2-0.src.rpm</a></td></tr>
<tr><td>Windows Setup :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast2_win32_2.3.2_setup.exe">icecast2_win32_2.3.2_setup.exe</a></td></tr>
</table>
</center>
<p>
<br />
<ul>
<li>Character set support.
Most non-Ogg streams (eg MP3) send metadata as Latin1 but it could be in other character sets.  As
icecast uses UTF-8, we need to convert to UTF-8 so that web pages and stream directories render
correctly.<br />
<ul>
  <li>Allow a per-mount &lt;charset&gt; setting.  For when source clients do not indicate which
  character set is in use.
  <li>a charset= parameter to the metadata request.
  <li>Default for non-Ogg content is now Latin-1 (aka ISO-8859-1).  Ogg content still uses UTF-8.
</ul><br />
<li>Authentication
<ul>
  <li>Each mountpoint authentication is isolated so one mount authentication does not slow another
  mountpoint authentication down.
  <li>XSL pages can require authentication.
  <li>Add some sanity checks for incorrect settings.
</ul><br />
<li>Listening socket update
<ul>
  <li>Allow multiple ports to be allocated which allow multiple shoutcast source clients to stream
  on the same icecast instance.
  <li>No internal limit on the number of listen sockets that can be used.
  <li>Allow for a listen socket to be marked SSL capable. While any client could connect on these,
  the idea is really for /admin requests.
  <li>allow for a &lt;listen-socket&gt; to bind to ipv4 and ipv6
</ul><br />
<li>XSL update
<ul>
  <li>XSL pages include an xspf link for non-auth streams.
  <li>XSL pages in both webroot and adminroot can take a mount= arg to limit which stats are
  transformed.
  <li>XSL files can specify the content-type to return to the client. It is no longer fixed to html
</ul><br />
<li>Updates for stream directory handling.
<ul>
  <li>A stream is not registered with the directory until after the first minute
  has passed. some streams have been stuck in a connect/disconnect loop.
  <li>Increase retry time on failure, if the stream add is rejected then it will
  be down to a misconfiguration so a long retry delay is performed.
  <li>A YP server that is not responding is ignored for a while
</ul><br />
<li>Updates for Win32.  Newer versions of dependency libs (libxml2/libxslt/libcurl etc). Buffer
overflow fixes when many streams are active
<li>Accept/Ban IP support.
Now takes filenames for explicitly accepting or denying requests from specified IP addresses.
Useful in cases where firewall access is not available.  The files, if defined, are re-read
automatically if updated.
<li>A Mountpoint is exported to the slaves even if no mount section is defined for
it. You can still use the hidden setting to prevent that from happening.
<li>Relays handle redirection (HTTP 302) if one is received at startup.
<li>Automatically generate XSPF playlist like we do with M3U, the mountpoint extension is .xspf
<li>Header updates for proxy handling and certain clients like some shoutcast source clients and
flash players.
<li>Added Kate/Skeleton codecs to Ogg handler.
<li>Various stats cleanups. Added some global stats (server ID, email contact etc).
<li>The streamlist passed from master to slave had a limited length, so if there were many streams
then some could of been left out.
<li>Documentation updates.
<li>Relay startup/shutdown is cleaner.
<li>several build cleanups.
<li>several resource leaks and race conditions fixed
</ul>
<div class="poster">
Posted June 2, 2008 by karl
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	
<br />
<br />

<div id="release_2.3.1" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.3.1</h3>
<p>
Downloads:<br></br>
<center>
<table border=0>
<tr><td>Source :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.1.tar.gz">icecast-2.3.1.tar.gz</a></td></tr>
<tr><td>SRPM    :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.1-0.src.rpm">icecast-2.3.1-0.src.rpm</a></td></tr>
<tr><td>Windows Setup    :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast2_win32_v2.3.1_setup.exe">icecast2_win32_v2.3.1_setup.exe</a></td></tr>
</table>
</center>
<p>
<font color=white size=4><b>New features for 2.3.1</b></font>
<p>
<font color=yellow>
new tag &lt;logsize&gt; in &lt;logging&gt; state the trigger size (in KB) for cycling the log files.
</font><br></br>
<br></br>
<font color=yellow>
new tag &lt;logarchive&gt; in &lt;logging&gt; enable (1) if you want to use a timestamp for an extension when cycling logs.
</font><br></br>
<br></br>
</p>
<p>
<font color=white size=4><b>Fixes for 2.3.1</b></font><br></br>
<font color=yellow>
Handling of certain shoutcast source clients is fixed, this typically affected NSV source clients
</font><br></br>
<br></br>
<font color=yellow>
A race in source shutdown when listeners are authenticated could lead to server crash
</font><br></br>
<br></br>
<font color=yellow>
An audio glitch was possible in playback of vorbis streams when a new logical stream started (eg metadata update).
</font><br></br>
<br></br>
<font color=yellow>
stats speedup. Processing large numbers of stats was slow. Typically only seen when using lots of streams on the same server.
</font><br></br>
<br></br>
<font color=yellow>
404 responses were being sent back in some places, now 403 is sent back.
</font><br></br>
<br></br>
<font color=yellow>
Auth URL now handles the authentication to scripts better.<br></br>
The order in which username/password are selected is<br></br>
  1. url is http://user:pass@host:port/....<br></br>
  2. &lt;param name="username"&gt;  and &lt;param name="password"&gt;<br></br>
  3. with listener_add/remove, listener supplied username/password is used.<br></br>
</font>
<br></br>
<font color=yellow>
A streams intro file can now be changed, using HUP,  on active streams.
</font><br></br>
<br></br>
<font color=yellow>
mount without a name crashed the server
</font><br></br>
<br></br>
<font color=yellow>
Various documentation updates
</font><br></br>
<br></br>
<font color=yellow>
</p>

<div class="poster">
Posted November 30, 2005 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	
<br></br>
<br></br>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>More Load Test Reports</h3>
<p>
We've done another round of load testing, this time going through a "large number of sources" test
as well as a comparison test with Shoutcast.
</p>
<p>All our load testing reports can be found <a href="loadtest.php">Here</a></p>
<div class="poster">
Posted November 14, 2005 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	
<br>
<br>

<div id="release_2.3.0" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.3.0</h3>
<p>We are pleased to announce the next release of Icecast.
</p>
<p>
Downloads:<br></br>
<center>
<table border=0>
<tr><td>Source :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.0.tar.gz">icecast-2.3.0.tar.gz</a></td></tr>
<tr><td>SRPM    :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast-2.3.0-0.src.rpm">icecast-2.3.0-0.src.rpm</a></td></tr>
<tr><td>Windows Setup    :</td><td><a href="http://downloads.xiph.org/releases/icecast/icecast2_win32_v2.3.0_setup.exe">icecast2_win32_v2.3.0_setup.exe</a></td></tr>
</table>
<i>please note that it may take a bit for the mirrors to propagate</i>.
</center>

<p>
<font color=white size=4><b>New features for 2.3.0</b></font>
<p>
<font color=yellow>
Streaming support for ogg speex, ogg flac, ogg midi
</font><br></br>
<br></br>
<br></br>
<font color=yellow>
intro file support - per mount settable
</font><br></br>
   Intro files will play when a listener first connects to a stream.  This
   is designed for station jingles and the like.  If you don't broadcast 
   in ogg vorbis, you must make sure the bitrate/samplerate/number of 
   channels match up to your stream.
<br></br>
<br></br>
<font color=yellow>
on-demand relays, global and per-relay settable
</font><br></br>
   On demand relays only connect to the relayed content when there are
   listeners attached to the relay.  This can save bandwidth in certain cases.
<br></br>
<br></br>
<font color=yellow>
fallback to file, extends on the intro file handling.
</font><br></br>
   With this feature, you can specify a "fallback file" which will be played
   in a loop and sent your currently connected listeners in the event of a
   source client disconnect.  This means your listeners stay connected while
   you fix your disconnect problem.  Same rules regarding bitrate/samplerate/
   number of channels apply as with intro files.
<br></br>
<br></br>
<font color=yellow>
new mount-level settings
</font><br></br>
1. public, type/subtype, genre settings, stream description,
      stream url, stream name, bitrate (override what is sent from the source
      client)<br></br>
2. mp3 metadata interval<br></br>
3. on-[dis]connect scripts can be stated per-mount, invoked at source 
      start/stop and take 1 arg which is the mountpoint.<br></br>
<br></br>
<br></br>

<font color=yellow>
New URL listener authenticator.
</font><br></br>
   This delegates your listener authorization to an external application.
   URL calls are made on listener connect/disconnect as well as source
   connect/disconnect.  It is meant for large broadcasters who have existing
   authentication systems that need to be integrated into.  Included is 
   an example php-based application that can be used in conjunction with 
   the url authenticator to manage a simple subscription-based broadcast.
<br></br>
<br></br>

<font color=yellow>
HTPasswd authenticator uses in-memory structures now.
</font><br></br>
<br></br>
<br></br>

<font color=yellow>
On demand files now can be fed through an authenticator
</font><br></br>
<br></br>
<br></br>

<font color=yellow>
Update to admin/web xslt interface
</font><br></br>
<br></br>
<br></br>

<font color=yellow>
Icecast can now be installed as a win32 service
</font><br></br>
<br></br>
<br></br>
</p>
<p>
<font color=white size=4><b>Fixes for 2.3.0</b></font><br></br>
* real/helix works<br></br>
* win32 access log correct<br></br>
* stats client is stable now (curl -X STATS http://admin@host:port/)<br></br>
* show mountpoints on stats that are inactive but have an active fallback<br></br>
* more updates over HUP possible<br></br>
* improved stability under heavy load<br></br>
* moving clients will no longer sometimes deadlock the server<br></br>
* avoid small writes to reduce TCP overhead.<br></br>
</p>

<div class="poster">
Posted September 23, 2005 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Load Test Report</h3>
<p>We've done a load test of our soon-to-be released Icecast 2.3 to hopefully answer some 
of the questions that have popped up recently about Icecast's ability to work in high 
traffic situtations.</p>
<p>This report can be found <a href="loadtest.php">Here</a></p>
<div class="poster">
Posted September 22, 2005 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div id="release_2.2.0" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.2.0</h3>
<p>On the heels of version 2.1.0, we are releasing 2.2.0 with some new key features.
</p>
<p>
<font color=white size=4><b>New features for 2.2.0 (in no particular order)</b></font>
<p>
<font color=yellow>
Theora Video support
</font><br></br>
Icecast now supports video streaming via theora.  Currently, we require the latest
(alpha 4) version of libtheora.  This is an optional compile, so if you don't
have theora then icecast will safely ignore it
<br></br>
<br></br>
<font color=yellow>
Shoutcast style source client support
</font><br></br>
Icecast now supports the connection protocol used by the Shoutcast DSP source
client.  This is the same connection protocol used by their NSV encoding tools.
This means that not only can you use the Shoutcast DSP to stream to icecast, but
that you can also stream NSV via their tools.
<br></br>
<br></br>
<font color=yellow>
AAC is added as a supported streaming format
</font><br></br>
Not too many source clients support streaming in this format, but we support it.
<br></br>
<br></br>
<font color=yellow>
Cluster password
</font><br></br>
Now you can specify a cluster password as a <mount> option in the config. This
will allow you to cluster multiple servers/mounts into a single listing on the
stream directory.  Note that this is different than "grouping" which groups together
streams coming from the same physical IP and with the same stream name.  Clusters
are meant for relays of the same stream and will only be listed *once* in the stream
directory.  When a listener tunes into a cluster, they will be served an m3u file
with all the clusters for that stream.
<br></br>
<br></br>
<font color=yellow>
Playlist Log
</font><br></br>
This is an option setting that will create an audit trail of metadata that comes through
icecast.  It is a single file that contains information for all mountpoints.
<br></br>
<br></br>
<font color=yellow>
Range Support for static files
</font><br></br>
We now support seeking in files served off the icecast fserve.
<br></br>
<br></br>
<font color=yellow>
Metadata Update via Admin
</font><br></br>
We now support metadata updates via the admin interface for both MP3 AND Ogg Vorbis
streams.
<br></br>
<br></br>

<font color=yellow>
Per mount hidden stats and YP prevention
</font><br></br>
You many now indicate certains mounts to be excluded (i.e. hidden) from the main
status.xsl page.  This is useful when using local private relays.  You can also
override the YP setting (as in disable) on a per-mount basis.  Also useful for
local private relays.
<br></br>
<br></br>
<font color=yellow>
Multiple example config files
</font><br></br>
We now have multiple config files for you to use as a base.  A "simple" one for
quick-start, and a more detailed "advanced" one with all the features, as well
as a "shoutcast compatable" one, which shows how you'd config for using the
shoutcast DSP.
<br></br>
<br></br>
<font color=yellow>
Relay user/pass
</font><br></br>
You can now specify authentication used by a relay.  This is for the case where
you have listener authentication enabled for a mountpoint, and want to connect
a relay to it.
<br></br>
<br></br>
</p>

<p>Download Icecast 2.2.0 from the <a href="download.php">Download page</a>.</p>

<div class="poster">
Posted December 21, 2004 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div id="release_2.1.0" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.1.0</h3>
<p>After about 8 months in development, version 2.1.0 of icecast has been released.
</p>
<p>
<font color=white size=4><b>New features for 2.1 (in no particular order)</b></font>
<p>
<font color=yellow>Listener Authentication</font><br></br>
  Icecast now supports listener authentication.  This provides a mechanism for
  creating/maintaining users and passwords for listeners.  Currently, we only
  have implemented a simple, file-based storage for users and passwords.  New
  authenticators are on the horizon (such as URL-based or possibly MySQL based)
  New admin pages were also added for the maintenance of users/passwords. Please
  check the docs for a more detailed description of this new feature.
<br></br>
<br></br>
<font color=yellow>Multi-Level Fallbacks</font><br></br>
  Multi-level fallbacks allow for specifications of a series of fallback mounts
  that you can use to automatically move listeners in the event of a source being
  disconnected.  In the event of new listeners attaching to the source stream that has 
  been disconnected, they are routed to the fallback mount (if specified).
  Icecast now has fallback-override capability as well which means that in the event
  of a source stream failure, listeners are moved to the fallback mount, and then
  automatically recaptured when the original source stream returns.
<br></br>
<br></br>
<font color=yellow>Burst-On-Connect</font><br></br>
  This is an new, optional config setting which will send a initial burst of
  data to connecting listeners.  This has the effect of reducing
  (significantly) the startup buffer latency from the end-user perspective.
  This option is enabled by default.
<br></br>
<br></br>
<font color=white size=4><b>New Enhancements for 2.1</b></font><br></br>
<font color=yellow>Update to admin interface</font><br></br>
  This interface has been cleaned up quite a bit and made a bit nicer.
<br></br>
<br></br>

<font color=yellow>Rewrite of the YP listing code</font><br></br>
  The icecast yp code has received a complete overhaul by karl, and it's a much
  more stable and failure-resistant implementation.
<br></br>
<br></br>

<font color=yellow>Lots and lots of bugs fixed</font><br></br>
  Check the ChangeLog for a complete list of these...
<br></br>
<br></br>
</p>

<p>Download Icecast 2.1.0 from the <a href="download.php">Download page</a>.</p>

<div class="poster">
Posted November 4, 2004 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div id="release_2.0.2" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.0.2</h3>
<p>This patch release fixes an exploit found by Luigi Auriemma. We recommend that all
users currently upgrade to this patch release.
</p>
<p>Download Icecast 2.0.2 from the <a href="download.php">Download page</a>.</p>

<div class="poster">
Posted September 24, 2004 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div id="release_2.0.1" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.0.1</h3>
<p>This patch release fixes a overflow buffer which can cause server crashes under
certain circumstances.  This release contains ONLY the fix for this issue.  We
are still targetting a 2.1.0 release with new features and functionality in the
near future.</p>

<p>Download Icecast 2.0.1 from the <a href="download.php">Download page</a>.</p>

<div class="poster">
Posted May 12, 2004 by oddsock
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>
<div id="ices_release_2.0.0" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Ices Release 2.0.0</h3>
<p>After far too long in development and testing, the icecast development team
has released Ices version 2.0.0.  Ices is a source client for Icecast streaming
media server. It takes audio from one of the defined inputs and (re)encodes the
audio for streaming to icecast for listeners to pick up.</p>

<p>Download Ices 2.0.0 from the <a href="ices.php">Ices page</a>.</p>

<div class="poster">
Posted Mar 11, 2004 by karl
</div> 
</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>

<div id="release_2.0.0" class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Releases 2.0.0</h3>
<p>After years in development and years in alpha testing, The icecast development team has released version 2.0.0 of its streaming media server.  Icecast2 supports Ogg Vorbis and MP3 streaming and has many features and functions you would expect from a world class streaming media server.</p>

<p>There are more things to come from the icecast development team, and we have many new features just waiting to be implemented.
</p>
<p>Download 2.0.0 from the main <a href="download.php">download page</a>.</p>
<p>In addition to the new release, we have re-vamped the icecast.org website to hopefully make things easier to access and to get at the information you need faster.  Please let us know what you think.</p>
<div class="poster">
Posted Jan 7, 2004 by oddsock
</div> 

</div>
<div class="roundbottom">
<img alt="" src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>	

<br>
<br>

