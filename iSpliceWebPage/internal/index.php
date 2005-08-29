<?php
include( "htm.clss");

#create objects
$sht      = new htm();
$message = "iSplice Annodex Creator";
$sht->html_begin($message,null);

#change top menus here if you need to
$header = "<h1>iSplice: The Annodex Creator (internal)</h1>";


$toplist =
         '
         <ul
             ><li class="hide"
                 ><a class="hide" href="../../">Hidden</a
                 ><span class="divider"> : </span
                 ></li
             ><li
                 ><a href="..">iSplice Home</a
                 ><span class="divider"> : </span
             ></li
             ><li
                 ><a href=".">Internal Home</a
                 ><span class="divider"> : </span
             ></li
         ></ul>
         ';

$sht->html_top($header, $toplist);

#do middle

$middle = '
        <h2>iSplice is here!</h2>
        <p>
        You are in the <strong>internal</strong> iSplice page.
        </p>

        <BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR>
        ';

$sht->html_middle($middle);

#do left column


$leftlist = '
          <div class="vnav">
              <h3>Internal</h3>

              <ul
                  ><li
                      ><a href="iSpliceList.php">Get iSplice Client List</a

                  ></li
              ></ul>

              <h3>Special</h3>

              <form action="../iSpliceLicense.php" method="POST" >
                              <input type="submit" Value="Get Special License"/>
                              <input type="hidden" name="SourceInput" Value="iSpliceAdministrator"/>
              </form>

          </div>
          ';

$sht->html_left($leftlist);

# right column

$sht->html_right(null);

#footer

$sht->html_footer();

?>