<? include "common/header.php"; ?>
<h2>Icecast Load Test Results #3 (by oddsock)</h2>
<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<br>
<div class="newscontent">
<h3>Description</h3>
<br></br>
<p>
This load test was performed to get an idea as to how well icecast performs relative to Shoutcast.  The test
used for this comparison is the "Max Listener" test that we used for load test #1.  In order to get the most
accurrate results, I decided to re-run the max listener test completely for icecast (instead of just taking the 
numbers from the previous test.
</p>

<h3>Test Hardware</h3>
<p>I used the same hardware as in the <a href="loadtest1.php">first load test</a> and <a href="loadtest2.php">second load test</a>.</p>
<p>
3 of these : (one for running icecast, and 2 for listeners)<br>
Server: <b>Dell Poweredge 1850</b><br></br>
Memory: <b>2GB</b><br></br>
CPU   : <b>3GHz Xeon (single processors running in hyperthreaded mode)</b><br></br>
Network: <b>2 GBit Ethernet (although only one was used for the testing) connected via a GBit ethernet switch.</b><br></br>
OS	  : <b>Red Hat Enterprise Linux 3 (2.4 kernel)</b><br></br>
</p>
<p>In both tests (icecast and Shoutcast) I used a single 48kbps mp3 stream sourced from Oddcast running on an XP machine on my local
LAN.</p>
<h3>The Load Test</h3>
<p>For this particular load test we used the same listener load script from the <a href="loadtest1.php">first load test</a>.
</p>
<p>
I ran this script of both of the listener nodes (each could start a max of ~7000 listeners) for a combined listener count of
~14000.  As in the previous tests, listeners were added in a "ramp-up" fashion up to the total of 14000 listeners.
</p>
<p>
After the execution of this test against both icecast and Shoutcast, I chose the following stats to compare against each server :<br></br>
<br></br>
* User CPU
<br></br>
* System CPU
<br></br>
* Free Memory
<br></br>
</p>
<p>These are machine-level statistics, and as before with previous tests, only icecast/Shoutcast were running on the main node
and thus all system metrics can be attributed to icecast/Shoutcast.</p>

<h4>User CPU</h4>
<p>The first graph we will look at is User CPU.  Listeners are along the X axis and User CPU along the Y.</p>
<center>
<img src="loadtest/LoadTest3_Icecast_vs_Shoutcast_UserCPU.png"><br></br>
</center>
<p>
This graph shows that icecast certainly uses less User CPU as compared to Shoutcast, taking up about 50% less cpu than Shoutcast does for
a similar listener count.  Icecast 1, Shoutcast 0.
</p>
<br></br>
<br></br>
<br></br>
<h4>System CPU</h4>
<p>Next, we will look at System CPU.</p>
<center>
<img src="loadtest/LoadTest3_Icecast_vs_Shoutcast_SystemCPU.png"><br></br>
</center>
<p>
This one is a little bit harder to draw a conclusion about.  Icecast and Shoutcast are pretty much identical System CPU-wise, until about
10,000 listeners, where we see a slight increase in System CPU time relatively for icecast.  However, if we look at the total 
graph of system CPU for icecast, we see a very linear progression.  With Shoutcast we see a plateau in System CPU right about 10,000
listeners.  Without going into more analysis I would conclude from this that Shoutcast is most likely not keeping up with the
listeners appropriately, and thus we do not see an increase in system CPU.  So we'll call this one a draw.
</p>
<br></br>
<br></br>
<br></br>
<h4>Free Memory</h4>
<p>And finally...Free Memory</p>
<center>
<img src="loadtest/LoadTest3_Icecast_vs_Shoutcast_freeMemory.png"><br></br>
</center>
<p>
Wow, this one is certainly striking.  Shoutcast must allocate a major chunk of memory for each listener or (shudder) allocate
a thread for each listener.  Either way, icecast clearly wins this one.
</p>
<br></br>
<br></br>
<br></br>

<h3>Conclusions</h3>
<p>
<center><i><b>How does icecast compare to Shoutcast in terms of handling listeners ?</b></i></center><br></br>
<p>
Well, looks like icecast handles them better all around taking about 50% of User CPU, about the same System CPU, and certainly
much much fewer amounts of memory per user.
</p>
<Br></br>
<Br></br>
<Br></br>
- oddsock : Mon Nov 14 12:43:56 CST 2005 - &copy; 2005 Ed Zaleski.
</p>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br><br>
