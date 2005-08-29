<?php

include( "htm.clss");

#create objects
$sht      = new htm();
$sht->html_begin("iSplice Annodex Creator",null);

$sht->html_top("<h1>iSplice: The Annodex Creator</h1>", null);

#<!--- middle (main content) column begin -->
$middle = "
<h2>iSplice Downloads</h2>
<p>
<table border=1 cellpadding=5>
<tr>
    <td>Version</td><td>Release Date</td><td>Link</td>
</tr>
<tr>
    <td>iSplice 1.10</td><td>20th August 2005</td><td><a href=\"./iSplice-1.10.zip\">download</a></td>
</tr>
</table>
</p>
<BR>
You will need a license key if you don't already have one.<BR>
<BR>
To get a license key you will need to enter the MAC address of the PC you will be installing the software on.<BR><BR>
If you know your MAC address you can go directly to the license page and enter it there. Alternatively, you can install 
the iSplice software, follow the links within the application to get a license, and we will fill in the MAC address for you.

<BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR>
";

$sht->html_middle($middle);

$sht->html_left(null);

#<!--- right column begin -->

$sht->html_right(null);

#<!--- right column end -->

$sht->html_footer();

?>