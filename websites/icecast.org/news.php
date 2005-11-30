<h2>News</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">
<h3>Icecast Release 2.3.1</h3>
<p>We are pleased to announce the next release of Icecast.
</p>
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

<div class="roundcont">
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
<div class="roundcont">
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
<div class="roundcont">
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
<div class="roundcont">
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
<div class="roundcont">
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
<div class="roundcont">
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

<div class="roundcont">
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

