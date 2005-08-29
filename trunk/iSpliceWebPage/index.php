<?php

include( "htm.clss");

#create objects
$sht      = new htm();
$sht->html_begin("iSplice Annodex Creator",null);

$sht->html_top("<h1>iSplice: The Annodex Creator</h1>", null);

#<!--- middle (main content) column begin -->
$middle = "
<h2>iSplice is here!</h2>
<p>
iSplice the Anoodex creator is now here and available for download. At this stage, 
30 day trial licenses are available, and we will be making full licenses available 
in the not too distant future.
</p>

<h2>Latest News</h2>
<p>
iSplice version 1.10 was released 20th August 2005.
</p>
<BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR>
";

$sht->html_middle($middle);

#do left
$leftlist = '
    <h3>Links</h3>
    <ul
        ><li
            ><a href="http://www.csiro.au">CSIRO</a
        ></li
        ><li
            ><a href="http://www.centie.net.au">CeNTie</a
        ></li
        ><li
            ><a href="http://www.annodex.net">Annodex</a
        ></li
    ></ul>
    ';


$sht->html_left($leftlist);

#do right
$rightlist = '
<p>
iSplice is a web authoring tool to help you create Annodex files. To learn more about Annodex
<a href="http://www.annodex.net">click here</a>.
</p>
<P>
The Current version of iSplice can be found <a href="./downloads.php?UI='.$_GET['UI'].'">here</a>
</p>
';

$sht->html_right($rightlist);

#do footer
$sht->html_footer();

?>