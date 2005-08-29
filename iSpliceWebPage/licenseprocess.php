<?php

include( "htm.clss");

#useful functions
function rijndael_encrypt($key, $encrypt) {
   srand((double) microtime() * 1000000); //for sake of MCRYPT_RAND
   $iv = mcrypt_create_iv(mcrypt_get_iv_size(MCRYPT_RIJNDAEL_128, MCRYPT_MODE_CBC), MCRYPT_RAND);
   // $iv = "0123456789012345";

   $passcrypt = mcrypt_encrypt(MCRYPT_RIJNDAEL_128, $key, $encrypt, MCRYPT_MODE_CBC, $iv);
   $encode = $iv.$passcrypt;

 return $encode;
 }

function rijndael_decrypt($key, $decrypt) {
   //$iv = "0123456789012345";

   $iv = substr($decrypt,0,16); // Get IV off the front of the message
   $message = substr($decrypt,16,strlen($decrypt)-16);

   $passcrypt = mcrypt_decrypt(MCRYPT_RIJNDAEL_128, $key, $message, MCRYPT_MODE_CBC, $iv);
   $decode = $passcrypt;
 return $decode;
 }

function hexbin($hex){
   $bin='';
   for($i=0;$i<strlen($hex);$i++)
       $bin.=str_pad(decbin(hexdec($hex{$i})),4,'0',STR_PAD_LEFT);
   return $bin;
} 

#create objects
$sht      = new htm();
$sht->html_begin("iSplice Annodex Creator",null);

$sht->html_top("<h1>iSplice: The Annodex Creator</h1>", null);

#<!--- middle (main content) column begin -->

   $middle = "<BR>";

   if ($email_address_1 != $email_address_2)
   {
      $middle .= 'The email addresses don\'t match';

      $sht->html_middle($middle);
      return;
   } 

   // Store the POST data more conveniently
   $MACAddress = $_POST['MACAddress'];
   $first_name = $_POST['first_name'];
   $email_address_1 = $_POST['email_address_1'];
   $email_address_2 = $_POST['email_address_2'];
   $country = $_POST['country'];
   $company = $_POST['company'];
   $ContactNumber = $_POST['ContactNumber'];
   $first_name = $_POST['first_name'];
   $last_name = $_POST['last_name'];
   $annodex_emails = $_POST['annodex_emails'];
   $admin_request = $_POST['admin_request'];

   if ($email_address_1 == NULL ||
      $email_address_2 == NULL ||
      $country == NULL ||
      $company == NULL ||
      $first_name == NULL ||
      $last_name == NULL)
   {
       $middle .= '<BR>Please fill in all required (*) fields<BR>';
       $sht->html_middle($middle);
       return;
   }

    $LicenseDays=$_POST[number_of_days];
    $LicenseUsageCount=$_POST[number_of_runs];
    $Product="iSplice";
    $Version=1.05;
    $ExpiryDate = date("Y-m-d");    // default to expiring today

//    $y=date("Y");
//    $m=date("m");
//    $d=date("d");
//    $d += $LicenseDays;
//    $ExpiryDate=date("Y-m-d",mktime(0,0,0,$m,$d,$y));

   //$middle .= '<p>Verify MAC Address</p>';
   $middle .= 'Mac Address: '.$MACAddress."<BR>";
   $middle .= '<BR>';
   $middle .= 'Name: '.$first_name.' '.$last_name."<BR>";
   $middle .= 'Company: '.$company."<BR>";
   $middle .= 'Country: '.$country."<BR>";
   $middle .= 'Email Address: '.$email_address_1."<BR>";

   $user = "anxcreator";
   $password = "csiro";
   $database = "AnxCreator";

   $mysql_access = mysql_connect("localhost", $user, $password) or die ("Couldn't connect to Database!");
   if (!mysql_select_db($database, $mysql_access))
   {
       // if it's not already there, create it.
       //mysql_create_db($database);
       $query = 'CREATE DATABASE '.$database;
       mysql_query($query, $mysql_access);

       mysql_select_db($database, $mysql_access) or die ("Failed to Create Database!");
   }

   $bGotLicenseTable = false;
   $result = mysql_query("SHOW tables", $mysql_access) or die ("Couldn't access Database Tables!");
   while($result && $row = mysql_fetch_row($result))
   {
       // $middle .= "$row[0]<br>";
       if (strcasecmp($row[0], "AnxCreatorLicenses") == 0)
       {
           $bGotLicenseTable = true;
       }
   }

   if ($bGotLicenseTable == false)
   {
       // not there, so create an empty table

       // Add: LicenseKey VARCHAR(255),

       $query = "CREATE TABLE AnxCreatorLicenses (
                    EntryRow INT AUTO_INCREMENT,
                    SerialNumber INT,
                    MACAddress VARCHAR(20),
                    IPAddress VARCHAR(20),
                    EmailAddress VARCHAR(255),
                    FirstName VARCHAR(64),
                    LastName VARCHAR(64),
                    CompanyName VARCHAR(64),
                    ContactNumber VARCHAR(32),
                    Country VARCHAR(32),
                    LicenseStartDate DATE,
                    LicenseExpiryDate DATE,
                    LicenseType VARCHAR(64),
                    LicenseUsagesAllowed INT,
                    LicenseVersion VARCHAR(16),
                    UpdateInfoRequested VARCHAR(1),
                    AnnodexInfoRequested VARCHAR(1),
                    PRIMARY KEY (EntryRow)
                    )";

       mysql_query($query, $mysql_access) or die ("Couldn't Create Database Table!");
   }

   // OK, by now we have a valid database and table. See if we already have a license for this MAC Address
   $query = "SELECT MACAddress,EmailAddress FROM AnxCreatorLicenses WHERE MACAddress='".$MACAddress."'";
   $result = mysql_query($query, $mysql_access);

   // admin requests can always add, no matter what was there before
   if ($admin_request == 'true' && $result && $row = mysql_fetch_row($result))
   {
       // it is true (the MAC Address exists)
       $middle .= "<strong>$MACAddress</strong> already exists in the database.<BR>";
       $middle .= "<BR><strong>License Addition Failed!</strong><BR>";
       $middle .= "Re-sending existing license to: ".$email_address_1."<BR>";

       $existing_email = false;
       do
       {
            //printf("Row:%s.%s<BR>", $row[0], $row[1]);
            if (strcasecmp ($row[1],$email_address_1) == 0)
            {
                $existing_email = true;
            }
       }
       while ($row = mysql_fetch_row($result));

       if ($existing_email == false)
       {
         $query = "SELECT EntryRow,MACAddress,EMailAddress,LicenseExpiryDate FROM AnxCreatorLicenses WHERE MACAddress = '".$MACAddress."'";
         $result = mysql_query($query, $mysql_access) or die ("Couldn't add to database");
         $row = mysql_fetch_row($result) or die ("Couldn't find row");

         // Only copy one of the rows to create our new row.
         $query = "INSERT INTO AnxCreatorLicenses (
                    SerialNumber,
                    MACAddress,
                    IPAddress,
                    FirstName,
                    LastName,
                    CompanyName,
                    ContactNumber,
                    Country,
                    LicenseStartDate,
                    LicenseExpiryDate,
                    LicenseType,
                    LicenseUsagesAllowed,
                    LicenseVersion,
                    UpdateInfoRequested,
                    AnnodexInfoRequested
                   ) SELECT
                    SerialNumber,
                    MACAddress,
                    IPAddress,
                    FirstName,
                    LastName,
                    CompanyName,
                    ContactNumber,
                    Country,
                    LicenseStartDate,
                    LicenseExpiryDate,
                    LicenseType,
                    LicenseUsagesAllowed,
                    LicenseVersion,
                    UpdateInfoRequested,
                    AnnodexInfoRequested
                   FROM AnxCreatorLicenses WHERE MACAddress = '".$MACAddress."' AND EntryRow = '".$row[0]."'";

         $result = mysql_query($query, $mysql_access) or die ("Couldn't update database");
         // and update.
         $query = "UPDATE AnxCreatorLicenses SET EmailAddress='".$email_address_1."'
                   WHERE MACAddress = '".$MACAddress."' AND EmailAddress IS NULL";
         $result = mysql_query($query, $mysql_access) or die ("Couldn't update database");
       }
         // and read the relevant information
         $query = "SELECT MACAddress,LicenseExpiryDate,LicenseType,LicenseUsagesAllowed FROM AnxCreatorLicenses WHERE MACAddress = '".$MACAddress."'";
         $result = mysql_query($query, $mysql_access) or die ("Couldn't add to database");
         $row = mysql_fetch_row($result) or die ("Couldn't find row");
         $ExpiryDate = $row[1];
         $LicenseDays = $row[2];
         $LicenseUsageCount = $row[3];

         $middle .= "Expiry: ".$ExpiryDate."<BR>";
         $middle .= "Days: ".$LicenseDays."<BR>";
         $middle .= "Usage: ".$LicenseUsageCount."<BR>";
   }
   else
   {
       // OK. MAC Address not already in Database. Add a new entry with a new Serial Number.
       $query = "Select Max(SerialNumber) FROM AnxCreatorLicenses";
       $result = mysql_query($query, $mysql_access);
       $SN = 0;
       if ($result && $row = mysql_fetch_row($result))
       {
               $SN = $row[0];
               $SN += 1;
       }
       //printf("Serial:%s\n",$SN);

       $y=date("Y");
       $m=date("m");
       $d=date("d");
       $StartDate = date("Y-m-d",mktime(0,0,0,$m,$d,$y));
       //$d += $LicenseDays;
       //$ExpiryDate=date("Y-m-d",mktime(0,0,0,$m,$d,$y));
       if ($LicenseDays != 0)
       {
           $ExpiryDate = date("Y-m-d",mktime(0,0,0,$m,$d+$LicenseDays,$y));
           $middle .= "Expiry: ".$ExpiryDate."<BR>";
       }
       else
       {
           $ExpiryDate = 0; // no expiry. Full license.
       }

       // trials currently limited to 50 uses
       $query = "INSERT INTO AnxCreatorLicenses
                 VALUES (
                 NULL,
                 '$SN',
                 '$MACAddress',
                 'IP Address',
                 '$email_address_1',
                 '$first_name',
                 '$last_name',
                 '$company',
                 '$ContactNumber',
                 '$Country',
                 '$StartDate',
                 '$ExpiryDate',
                 '$LicenseDays',
                 '$LicenseUsageCount',
                 NULL,
                 NULL,
                 '$annodex_emails'
                 )";
                 
       // print($query);    // For debugging only

       $result = mysql_query($query, $mysql_access);
       if (!$result)
       {
              $middle .= "<BR><strong>License Addition Failed!</strong> Contact administrator<BR>";
              mysql_close($mysql_access);

              $sht->html_middle($middle);
              return;
       }
       else
       {
               $middle .= "<BR><strong>License Addition Successful!</strong><BR>";
               $middle .= "Sending license information to: ".$email_address_1."<BR>";
       }
   }

   mysql_close($mysql_access);

    $message = $MACAddress.','.$Product.','.$Version.','.$LicenseDays.','.$LicenseUsageCount.','.$ExpiryDate;


    // And encode it
    // gives us a 128 bit key. Ideal for Rijndael 128. 1 means give it as raw binary.
    // Result is: 0xd014c9f49a972c0b2493dfbe830b67aa
    $keyprep = "CSIRO ICT Centre";
    // PHP4 used on riddick.cent.net.au doesn't allow a second parameter in md5
    // so, use sinlge paramter then convert it
    //$key = md5($keyprep, 1);
    $keymd5 = md5($keyprep);
    $key = pack("H*", $keymd5);

    //print("Key is: $key<BR>");
    
    //$key = "CSIRO ICT Centre";

    //$middle .= "<BR><BR>$key<BR><BR>";

    $enc = rijndael_encrypt($key, $message);
    $enc64 = base64_encode($enc);
    //$middle .= "<BR>RIJNDAEL Test Encrypted: ".$enc64."<BR>";

    // Test decrypt
    //$dec64 = base64_decode($enc64);
    //$dec = rijndael_decrypt($key, $dec64);
    //$middle .= "<BR>RIJNDAEL Test Decrypted: ".$dec."<BR>";

    $MailSubject = "CSIRO Product License Key for ".$Product;
    $MailBody = "Please cut and paste the license key attached into the license key entry field in the application.\n\nLicense Key:\n\n".
                 $enc64."\n";

    //print ($MailSubject);
    //$middle .= "<BR>";
    //print ($MailBody);

    if (!mail($email_address_1,$MailSubject,$MailBody))
    {
        $sht->html_middle("Couldn't send email");
        return;
    }

    $middle .= "Your license key has been emailed to ".$email_address_1.".<BR>";
    $middle .= "Please cut and paste the license key from the email into the license key entry field in the application.<BR>";

    $middle .= "<BR><BR><BR><BR><BR><BR><BR><BR><BR><BR>";

    $sht->html_middle($middle);

#do left column

$sht->html_left(null);

# right column

$sht->html_right(null);

#footer

$sht->html_footer();

?>