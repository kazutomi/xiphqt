<? include "inc/header.php"; ?>
<h2>Icecast Load Test Results #2 (by oddsock)</h2>
<div class="roundcont">
<div class="roundtop">
<img src="/images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<br>
<div class="newscontent">
<h3>Description</h3>
<br></br>
<p>
Ok, here we go with another icecast load test.  In this test we are going to try to answer the following questions :<br></br>
* <b>How does adding additional sources affect icecast ? 
     We will look at a few metrics to try to answer this as complete as possible.</b><br></br>
* <b>Are there any limits to the number of sources that can be hosted on a single icecast instance ?</b><br></br>
* <b>What happens if we add a bunch of listeners to a high-source-count setup.</b><br></br>
</p>

<h3>Test Hardware</h3>
<p>I used the same hardware as in the <a href="loadtest1.php">first load test</a>.</p>
<p>
2 of these : (one for running icecast, and one for sources/listeners)<br>
Server: <b>Dell Poweredge 1850</b><br></br>
Memory: <b>2GB</b><br></br>
CPU   : <b>3GHz Xeon (single processors running in hyperthreaded mode)</b><br></br>
Network: <b>2 GBit Ethernet (although only one was used for the testing) connected via a GBit ethernet switch.</b><br></br>
OS	  : <b>Red Hat Enterprise Linux 3 (2.4 kernel)</b><br></br>
</p>
<h3>The Load Test(s)</h3>
<p>For this particular load test we used the same listener load script from the <a href="loadtest1.php">first load test</a>, 
and added a new script for creating source clients.
<p>We created mountpoints with the following script:</p>
<p>
<pre>
          #!/bin/sh
          #
          max=700
          # how long to sleep between each set of ezstreams, can be decimal  0.5
          delay=2
          
          echo &gt; out
          count=1
          while [ "$count" -le "$max" ]
          do
                  ezstream -c conf/ezstream_mp3_$count.xml &gt;&gt; out &amp;
                  ezstream -c conf/ezstream_vorbis_$count.xml &gt;&gt; out &amp;
                  let count=$count+1
                  ezstream -c conf/ezstream_mp3_$count.xml &gt;&gt; out &amp;
                  ezstream -c conf/ezstream_vorbis_$count.xml &gt;&gt; out &amp;
                  let count=$count+1
                  ezstream -c conf/ezstream_mp3_$count.xml &gt;&gt; out &amp;
                  ezstream -c conf/ezstream_vorbis_$count.xml &gt;&gt; out &amp;
                  let count=$count+1
                  ezstream -c conf/ezstream_mp3_$count.xml &gt;&gt; out &amp;
                  ezstream -c conf/ezstream_vorbis_$count.xml &gt;&gt; out &amp;
                  let count=$count+1
                  ezstream -c conf/ezstream_mp3_$count.xml &gt;&gt; out &amp;
                  ezstream -c conf/ezstream_vorbis_$count.xml &gt;&gt; out &amp;
                  let count=$count+1
              [ "$delay" != "" ] &amp;&amp; sleep $delay
          done
          wait
          echo done
</pre>
</p>
<p>
This script creates 10 new source client connections (5 mp3 and 5 vorbis) every 2 seconds.  The config files (exstream_*.xml)
were all precreated and were all identical with the exception of the mountpoint (which was a counter from 1 to 1400).
These ezstream instances looped the same file over and over (metadata updates were perform in between each loop - this
is automatically done by ezstream).
</p>

<p><b><font size=+1 color="yellow">First Test: Source Client Ramp Up To 1400</font></b></p>

<p>
The idea here is to ramp up (in a regular fashion) the number of source clients to a value of 1400.  
Why 1400 ? Well, 1400 sources ended up with a VSZ of 1GB, and while I probably could have gone higher, the purpose of
this test was not to see the max number of sources that *can* be attached, but rather can a large number of them be
attached without icecast saturating somewhere, and what kind of memory/cpu was taken up by such a "large number" source test.
</p>
<p>
So let us try to determine how much memory is taken by each source client.  
We will measure this by looking at the VSZ value for icecast (as reported by ps -aux).
</p>

<p>The first graph shows VSZ and source count.</p>
<img src="loadtest/LoadTest2_VSZ_vs_sources.png"><br></br>
<p>
<br></br>
So lets take a look at some of the raw data, here is the VSZ and source number values, along with a delta 
representing the different between the current and previous VSZ value divided by the number of sources that were added.
This effectively represents the KB overhead per source. <i>Note that VSZ is reported in KB.</i>
</p>
<br></br>
<center>

<table border=1 width="80%" cellspacing=0 cellpadding=0>
<tr><td bgcolor="#111111"><b>VSZ Icecast</b></td><td bgcolor="#111111"><b>Number of Sources</b></td><td bgcolor="#111111"><b>Delta per source</b></td></tr>
<tr><td>10648</td><td>0.00</td><td></td></tr>
<tr><td>21512</td><td>20.00</td><td>543.20</td></tr>
<tr><td>28012</td><td>30.00</td><td>650.00</td></tr>
<tr><td>41836</td><td>50.00</td><td>691.20</td></tr>
<tr><td>48308</td><td>60.00</td><td>647.20</td></tr>
<tr><td>61388</td><td>80.00</td><td>654.00</td></tr>
<tr><td>68008</td><td>90.00</td><td>662.00</td></tr>
<tr><td>79772</td><td>110.00</td><td>588.20</td></tr>
<tr><td>88440</td><td>120.00</td><td>866.80</td></tr>
<tr><td>100336</td><td>140.00</td><td>594.80</td></tr>
<tr><td>107704</td><td>150.00</td><td>736.80</td></tr>
<tr><td>118568</td><td>170.00</td><td>543.20</td></tr>
<tr><td>128520</td><td>180.00</td><td>995.20</td></tr>
<tr><td>140900</td><td>200.00</td><td>619.00</td></tr>
<tr><td>147112</td><td>210.00</td><td>621.20</td></tr>
<tr><td>158020</td><td>230.00</td><td>545.40</td></tr>
<tr><td>168340</td><td>240.00</td><td>1032.00</td></tr>
<tr><td>180912</td><td>260.00</td><td>628.60</td></tr>
<tr><td>186864</td><td>270.00</td><td>595.20</td></tr>
<tr><td>200940</td><td>290.00</td><td>703.80</td></tr>
<tr><td>207288</td><td>300.00</td><td>634.80</td></tr>
<tr><td>219812</td><td>320.00</td><td>626.20</td></tr>
<tr><td>227348</td><td>330.00</td><td>753.60</td></tr>
<tr><td>234200</td><td>350.00</td><td>342.60</td></tr>
<tr><td>247384</td><td>360.00</td><td>1318.40</td></tr>
<tr><td>254232</td><td>380.00</td><td>342.40</td></tr>
<tr><td>267556</td><td>400.00</td><td>666.20</td></tr>
<tr><td>274164</td><td>410.00</td><td>660.80</td></tr>
<tr><td>287348</td><td>430.00</td><td>659.20</td></tr>
<tr><td>294196</td><td>440.00</td><td>684.80</td></tr>
<tr><td>306760</td><td>460.00</td><td>628.20</td></tr>
<tr><td>313476</td><td>470.00</td><td>671.60</td></tr>
<tr><td>327072</td><td>490.00</td><td>679.80</td></tr>
<tr><td>334944</td><td>500.00</td><td>787.20</td></tr>
<tr><td>346900</td><td>520.00</td><td>597.80</td></tr>
<tr><td>354636</td><td>530.00</td><td>773.60</td></tr>
<tr><td>366304</td><td>550.00</td><td>583.40</td></tr>
<tr><td>374184</td><td>560.00</td><td>788.00</td></tr>
<tr><td>387084</td><td>580.00</td><td>645.00</td></tr>
<tr><td>394820</td><td>600.00</td><td>386.80</td></tr>
<tr><td>406204</td><td>610.00</td><td>1138.40</td></tr>
<tr><td>414984</td><td>630.00</td><td>439.00</td></tr>
<tr><td>427420</td><td>640.00</td><td>1243.60</td></tr>
<tr><td>434136</td><td>660.00</td><td>335.80</td></tr>
<tr><td>447712</td><td>670.00</td><td>1357.60</td></tr>
<tr><td>454556</td><td>690.00</td><td>342.20</td></tr>
<tr><td>466984</td><td>700.00</td><td>1242.80</td></tr>
<tr><td>474740</td><td>720.00</td><td>387.80</td></tr>
<tr><td>488184</td><td>740.00</td><td>672.20</td></tr>
<tr><td></td><td></td><td></td></tr>
<tr><td></td><td bgcolor="#111111"><b>Average Delta Per Source</b></td><td bgcolor="#111111"><b>693.91</b></td></tr>
</table>

</center>

<p>Ok, that's probably enough data to make a conclusion.  Looks like each new addition of a source client is 
causing an increase of (on average) 672.20KB of memory in the icecast process.  This
means we'd expect an increase of 1400*.672 = .95GB in the VSZ for icecast compared to its initial value.  From our graph we can see
the initial value of ~ 15MB and a final value of .95GB for a net increase in the range that we were expecting.
</p>

<p>Next let us look at CPU used by icecast.  The following graph shows the CPU utilization (user/system/wait io) for the box running icecast. 
It is important
to note that only icecast was running on this box, so any cpu activity is associated with icecast.
Also note that initially, I used PCT_CPU as returned by 'ps -augx', however it was determined that
this value was not representative of actual PCT_CPU of the process.  Thus I opted to used the cpu 
utilization of the machine itself to get a more accurate representation.
</p>
<center>
<img src="loadtest/LoadTest2_UserSystemIOWait_vs_sources.png"><br></br>
</center>
<br></br>
<p>From this graph you can see that total cpu went from about 5% (2.5% user/ 2.5% system) up to about 20% (10% user / 10% system) 
during the course of the sources attaching.
There is a general upward trend in increasing CPU time, however, it is fairly regular and relatively small.
So we'd conclude that icecast is doing a very efficient job in handling this number of sources.  
<br></br>
<br></br>
<p><b><font size=+1 color="yellow">Second Test: Adding Listeners to the 1400 source client config</font></b></p>
<p>This test is really a combination of the first test I did (max listeners) and this test (max sources) to 
see how the system reacts to a system with a good number of listeners and sources combined.  To start off, source
clients were ramped up to 1400 (just like in the <b>First Test</b>), and then I ramped up to 5600 concurrent listeners.
These listeners were randomly distributed across the 1400 available attached mountpoints and connected and stayed 
connected for the duration of the test.  Why 5600 ? The machine that I was running listeners on ended up running out of resource
(number of processes) at that point.</p>
<p>Let us first take a look at the icecast process VSZ (Virtual Memory Size).</p>
<br></br>
<center>
<img src="loadtest/LoadTest2_VSZ_vs_sources_and_listeners.png"><br></br>
</center>
<p>In the above diagram, the sources are represented by a red line and listeners by a blue (red shaded area is the VSZ of icecast).  
As can be seen, the VSZ of 
the icecast process significantly increases with the addition of new sources (this is something that we found and discussed 
above in this document) but that it remains flat upon the addition of new listeners.  This is good news for those
of you who want to have a low memory footprint but still want a large number of listeners.</p>
<br></br>
<p>Let us now look at system CPU as a function of these two metrics (source count and listener count).  The next graph shows
system CPU utilization (User/System/IOWait) as reported by vmstat.</p>
<br></br>
<center>
<img src="loadtest/LoadTest2_UserSystem_vs_sources_and_listeners.png"><br></br>
</center>
<p>The effects are fairly subtle in this diagram, but what is shown is a relatively constantly increasing cpu 
utilization during the source client rampup, and a similar increase in CPU utilization
during the listener ramp up.  The slope of the line representing the increase in CPU utilization seems to be steeper
with the source ramp up as compared to the listener ramp up.  This would imply that the addition of new sources will imply
a larger CPU utilization than the addition of new listeners.  This is exactly what we would expect.
<p>So what did we learn from this test ? We saw that icecast memory does not appreciably increase with the addition of 
listeners (which it does with the addition of sources).  We also saw that CPU utilization is slightly higher for processing
sources than listeners.</p>
</p>
<h3>Conclusions</h3>
<p>
<center><i><b>How does adding additional sources affect icecast ?</b></i></center><br></br>
<p>We found that each source client adds about 672KB to the virtual size of the icecast process.  This ends up being the
the limiting factor when determining the number of sources you can host on a single icecast instance.</p>
<center><i><b>Are there any limits to the number of sources that can be hosted on a single icecast instance ?</b></i></center><br></br>
<p>Certainly there are, I ran into a limit of 1400 on the hardware I used, with the addition of additional RAM I would have
been able to increase that value.</p>
<center><i><b>What happens if we add a bunch of listeners to a high-source-count setup.</b></i></center><br></br>
<p>We found that listeners do not seem to affect icecast too much with regard to memory, and that it adds 
less load to the CPU as do sources.  In the case of listeners, the network will be your limiting factor (which we
found and reported in our previous load test).</p>
</p>
<Br></br>
<Br></br>
<Br></br>
- oddsock : Sat Nov 12 10:44:00 CST 2005 - &copy; 2005 Ed Zaleski.
</p>
</div>
<div class="roundbottom">
<img src="/images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>
</div>
<br><br>
