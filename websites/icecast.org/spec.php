<? include "common/header.php"; ?>
<h2>Icecast Directory Listing Specification</h2>
<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>

<br>
<div class="newscontent">
<h3>Description</h3>
<br></br>
<p>This document will describe the proposed protocol and components to be used for building a directory server (YP Server) for icecast2 and possibly other servers as well.  This document will be a work in progress.
<h3>Overview</h3>
<p>The need to centrally list streaming broadcasts has been done a bunch of times before with moderate degrees of success.  This approach tries to take the good parts of each and hopefully create a better incarnation.  The key aspect of this system will be simplicity.  Simplicity, in the protocol and maintenance of the system, will take precedence in the design process.</p>

<p>
Examples of existing directories can be found at :<br></br>
* <a href="http://www.shoutcast.com">Shoutcast</a><br></br>
* <a href="http://www.live365.com">Live365</a><br></br>
* <a href="http://wp.casterclub.com">CasterClub</a><br></br>
* <a href="http://www.radiotoolbox.com/dir">Radio Toolbox</a><br></br>
</p>
<h3>Basics</h3>
<p>To support a central directory listing of broadcasts, the minimum capabilities are required :</p>
<p>
* Ability to create a new entry in the listing<br></br>
* Ability to update an existing entry in the listing<br></br>
* Ability to remove an entry from the listing<br></br>
* Ability to group related broadcasts<br></br>
* Ability to cluster relayed broadcasts into a single listing<br></br>
* Support for icecast/icecast2 mountpoints<br></br>
* Listing should be done by the streaming server (i.e. no manual effort required)<br></br>
* Listings should not require registration or other prior sign-up<br></br>
</p>
<br>
<p>Additionally, the protocols which define the conversations above should NOT be proprietary, and should be well defined.</p>
<br>
<h3>Protocol</h3>
<p>HTTP protocol will be used for all communication between the listing client (icecast2 in this case) and the listing server (listing script(s))</p>
<p>
A few notes about the protocol before you get into it....Listener counts will be provided by the listing client, however, the current plan is for this information to not be displayed in the actual YP.  While this was at first a very difficult thing to decide upon (listener counts are always desirable), the tenets of "Simplicity" overrode the desire for this information.  The reasoning is that if listener information is stored at the listing server, then people will want to rate broadcasts based off this information.  Due to the "open" nature of the protocol, this information can be very easily faked (and has been in the past).  So, in order to make hosting a directory server a nice easy task, and not a full-time policing job, metrics such as listener count have been excluded.  Any ideas regarding this are welcomed.
</p>

<p>
The listing client will make HTTP requests on the listing server.  The following types of requests will be supported :
</p>

<h3>Add Server</h3>
<p>
This type of request will add a new server entry to the directory.  
</p>

<p>
The URL call will have the following mandatory parameters :
</p>
<center><table border=1 width="80%">
<tr><td>action</td><td>Action (add in this case)</td></tr>
<tr><td>sn</td><td>Server Name</td></tr>
<tr><td>type</td><td>Server Type (content type)</td></tr>
<tr><td>genre</td><td>Server Genre</td></tr>
<tr><td>b</td><td>Server Bitrate</td></tr>
<tr><td>listenurl</td><td>Listen URL (url which listeners use to listen)</td></tr>
</table>
</center>
<p>
The URL call will have the following *optional* parameters :
</p>
<center>
<table border=1 width="80%">
<tr><td>cpswd</td><td>Cluster Password (broadcasts with the same Server Name and cluster password will be displayed together in the directory server).</td></tr>
<tr><td>user</td><td>YP userid - not necessilarily all YP implementation will support users and passwords</td></tr>
<tr><td>pass</td><td>YP password - not necessilarily all YP implementation will support users and passwords</td></tr>
<tr><td>desc</td><td>Server Description</td></tr>
<tr><td>url</td><td>Stream URL (not the listen url, usually a link to the broadcasters website)</td></tr>
<tr><td>stype</td><td>Server Sub type.  Used normally for multi-codec streams (ogg/theora, vp6/aac).  Codecs should be separated by a '/' delimiter.</td></tr>
</table>
</center>
<p>
The listing scripts will respond with the following HTTP headers
</p>
<center>
<table border=1 width="80%">
<tr><td>YPResponse:</td><td>(0-failure or 1-success)</td></tr>
<tr><td>YPMessage:</td><td>Any error message</td></tr>
<tr><td>SID:</td><td>System Identifier which represents the unique identifier for the new listing entry.  All futher communications must be made using this SID - the SID can be any alpha numeric string</td></tr>
<tr><td>TouchFreq:</td><td>The frequency (in seconds) in which the listing client needs to touch the server in order to prevent a stale record</td></tr>
</table>
</center>
<p>
The following is an example of the communication between the listing client and listing server
</p>
<p>
<table border=0 width="70%">
<tr><td>
<pre>
URL: http://dir.xiph.org/cgi-bin/yp-cgi?action=add&amp;sn=Oddsock+Server&amp;
     listenurl=http://localhost:8000/oddsock.ogg&amp;genre=Rock&amp;b=64&amp;
     type=Ogg+Vorbis

Listing Client Request
-----------------
GET /yp-cgi?action=add&amp;sn=Oddsock+Server&amp;listenurl=http://localhost:8000/oddsock.ogg&amp;<br>genre=Rock&amp;b=64&amp;type=application/ogg


Listing Server Response
-----------------
HTTP/1.0 200 OK
YPResponse: 1
YPMessage: Successfully Added
SID: 1041755216.363775
TouchFreq: 60

OR

HTTP/1.0 200 OK
YPResponse: 0
YPMessage: Problem with blah blah
YPSID: -1
</pre>
</td></tr></table>


</p>


<h3>Touch Server</h3>
<p>
This type of request will update a server entry with new information. This request will also cause the listing server to acknowledge this server as one that is still valid.  Periodic cleanups of inactive servers will be performed on the listing server.
</p>

<p>
The URL call will have the following mandatory parameters :
</p>
<center><table border=1 width="80%">
<tr><td>action</td><td>Action (touch in this case)</td></tr>
<tr><td>sid</td><td>System Identifier received from the Add Server call</td></tr>
</table>
</center>
<p>
The URL call will have the following *optional* parameters :
</p>
<center><table border=1 width="80%">
<tr><td>st</td><td>Song Title</td></tr>
<tr><td>listeners</td><td>Number of listeners ***</td></tr>
<tr><td>max_listeners</td><td>Maximum capacity of station ***</td></tr>
<tr><td>alt</td><td>Average Listening time ***</td></tr>
<tr><td>ht</td><td>Stream Hits (tuneins) ***</td></tr>
<tr><td>cm</td><td>5 minute tunein ***</td></tr>
<tr><td>stype</td><td>Server Sub type.  Used normally for multi-codec streams (ogg/theora, vp6/aac).  Codecs should be separated by a '/' delimiter. - Note that since it is possible to change codecs mid stream in some container formats, so this field is updatable on a touch.</td></tr>
</table>
<p>
*** = due to the open-source nature of icecast, these metrics may not be reliable (i.e. they may be fake), it is up to the YP administrator to decide whether to include these stats.
</p>
</center>
<p>
The listing scripts will respond with the following HTTP headers
</p>
<center><table border=1 width="80%">
<tr><td>YPResponse:</td><td>(0-failure or 1-success)</td></tr>
<tr><td>YPMessage:</td><td>Any error message</td></tr>
</table>
</center>
<p>
The following is an example of the communication between the listing client and listing server
</p>
<p>
<table border=0 width="70%">
<tr><td>
<pre>
URL: http://dir.xiph.org/cgi-bin/yp-cgi?action=touch&amp;sid=1041755216.363775&amp;<br>st=Metallica-MasterOfPuppets

Listing Client Request
-----------------
GET /yp-cgi?action=touch&amp;sid=1041755216.363775&amp;st=Metallica-MasterOfPuppets


Listing Server Response
-----------------
HTTP/1.0 200 OK
YPResponse: 1
YPMessage: Updated Server Info

OR

HTTP/1.0 200 OK
YPResponse: 0
YPMessage: We had a problem...


</pre>
</td></tr></table>
</p>
<h3>Remove Server</h3>
<p>
This type of request will remove a server entry from the Directory.  This request should be done when either the broadcast has stopped, or the icecast2 server has stopped.
</p>

<p>
The URL call will have the following mandatory parameters (all parameters are mandatory) :
</p>
<center><table border=1 width="80%">
<tr><td>action</td><td>Action (remove in this case)</td></tr>
<tr><td>sid</td><td>System Identifier received from the Add Server call</td></tr>
</table>
</center>
<p>
The listing scripts will respond with the following HTTP headers
</p>
<center><table border=1 width="80%">
<tr><td>YPResponse:</td><td>(0-failure or 1-success)</td></tr>
<tr><td>YPMessage:</td><td>Any error message</td></tr>
</table>
</center>
<p>
The following is an example of the communication between the listing client and listing server
</p>
<p>
<table border=0 width="70%">
<tr><td>
<pre>
URL: http://dir.xiph.org/cgi-bin/yp-cgi?action=remove&amp;sid=1041755216.363775

Listing Client Request
-----------------
GET /yp-cgi?action=remove&amp;sid=1041755216.363775


Listing Server Response
-----------------
HTTP/1.0 200 OK
YPResponse: 1
YPMessage: Deleted Server Info

OR

HTTP/1.0 200 OK
YPResponse: 0
YPMessage: We had a problem...

</pre>
</td></tr></table>
</p>
<h3>Server Implementation</h3>
<p>
This section describes my thoughts on the implementation of the listing server part of the system.  This is the server script(s) which accept in the Add/Touch/Delete calls from icecast2 and process them.
</p>
<p>
Here are some requirements that I put together regarding this component :
</p>

<p>
* It needs access to a DB (probably mySQL due to it's common use and the fact that it's free :)<br></br>
* It needs to be fast (the scripts shouldn't be scripts, but probably a C program)<br></br>
* It needs to handle a potential large amount of traffic (this means not only does it have to be fast, but it has to scale up pretty good to high loads)<br></br>
* It needs to be robust (yeah, well, thought I'd throw that in anyway).<br></br>
</p>
<p>
Based off these requirements, I suggest that the server listing scripts be implemented as FastCGI C programs that access a mySQL to store the listing information.  This database will also need to be accessed by other scripts (PHP/ASP/etc.) which will front-end the entire directory (directory browser).  FastCGI is recommended due to the fact that it can be used to pool mySQL DB connections on the server and also hopefully provide the speed and scalability need to meet the requirements.
</p>
<h3>mySQL Database Schema</h3>
<p>
<table border=0 width="70%">
<tr><td>
<pre>

--
-- Table structure for table 'server_details'
--

CREATE TABLE if not exists server_details (
  id mediumint(9) NOT NULL auto_increment,
  parent_id mediumint(9) default NULL,
  server_name varchar(100) default NULL,
  listing_ip varchar(25) default NULL,
  description varchar(255) default NULL,
  genre varchar(100) default NULL,
  sid varchar(200) default NULL,
  cluster_password varchar(50) default NULL,
  url varchar(255) default NULL,
  current_song varchar(255) default NULL,
  listen_url varchar(200) default NULL,
  playlist_id mediumint(9) default NULL,
  server_type varchar(25) default NULL,
  server_subtype varchar(255) default NULL,
  bitrate varchar(25) default NULL,
  listeners int(11) default NULL,
  channels varchar(25) default NULL,
  samplerate varchar(25) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

create table if not exists playlists (
  id mediumint(9) NOT NULL,
  listen_url varchar(200) default NULL
) TYPE=MyISAM;

create table if not exists clusters (
  id mediumint(9) NOT NULL auto_increment,
  server_name varchar(255) default NULL,
  cluster_password varchar(50) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Table structure for table 'servers'
--

CREATE TABLE if not exists servers (
  id mediumint(9) NOT NULL auto_increment,
  server_name varchar(100) default NULL,
  listing_ip varchar(25) default NULL,
  listeners int(11) default NULL,
  rank int(11) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Table structure for table 'servers_touch'
--

CREATE TABLE if not exists servers_touch (
  id varchar(200) NOT NULL default '',
  server_name varchar(100) default NULL,
  listing_ip varchar(25) default NULL,
  last_touch datetime default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;



</pre>
</td></tr></table>
</p>
<h3>Closing</h3>
<p>
The purpose of this document was to start a basic specification for the directory listing that is badly needed in icecast2.  It is by no means expected to be a complete document.  All comments are welcome. :)
</p>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br><br>
