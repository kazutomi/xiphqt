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

   $middle = null;

   $user = "anxcreator";
   $password = "csiro";
   $database = "AnxCreator";

   $mysql_access = mysql_connect("localhost", $user, $password) or die ("Couldn't connect to Database!");
   mysql_select_db($database, $mysql_access) or die ("Failed to access Database!");

   $bGotLicenseTable = false;
   $result = mysql_query("SHOW tables", $mysql_access) or die ("Couldn't access Database Tables!");
   while($result && $row = mysql_fetch_row($result))
   {
       $middle .= "$row[0]<br>";
       if (strcasecmp($row[0], "AnxCreatorLicenses") == 0)
       {
           $bGotLicenseTable = true;
       }
   }

   // OK, by now we have a valid database and table. See if we already have a license for this MAC Address
   $query = "SELECT EntryRow, SerialNumber, MACAddress, LicenseStartDate, LicenseExpiryDate, LicenseType, EmailAddress FROM AnxCreatorLicenses";
   //$query = "SELECT * FROM AnxCreatorLicenses";
   $result = mysql_query($query, $mysql_access);
   if ($result && $row = mysql_fetch_row($result))
   {
       do
       {
            $middle .= "Row:$row[0], $row[1], $row[2], $row[3], $row[4], $row[5], $row[6]<BR>";
       }
       while ($row = mysql_fetch_row($result));

   }
   mysql_close($mysql_access);

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