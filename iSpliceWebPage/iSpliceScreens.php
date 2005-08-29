<?php
include( "htm.clss");

#create objects
$sht      = new htm();
$message = "iSplice Annodex Creator";
$sht->html_begin($message,null);

#change top menus here if you need to
$header = "<h1>iSplice: The Annodex Creator</h1>";

$sht->html_top($header, null);

#<!--- middle (main content) column begin -->
$middle = "
        <h2>iSplice Screen Shots</h2>
        <p>
        <img src=isplice1.jpg>
        </p>
        <BR>
        <BR><BR><BR><BR><BR><BR><BR><BR>
        ";

$sht->html_middle($middle);

#do left column

$sht->html_left(null);

# right column

$sht->html_right(null);

#footer

$sht->html_footer();

?>