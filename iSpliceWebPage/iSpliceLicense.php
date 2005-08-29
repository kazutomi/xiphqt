<?php
include( "htm.clss");

#create objects
$sht      = new htm();
$message = "iSplice Annodex Creator";
$sht->html_begin($message,null);

#change top menus here if you need to
$header = "<h1>iSplice: The Annodex Creator</h1>";

$sht->html_top($header, null);

#do middle

$message = '
<p>Welcome to the iSplice Registration Page</p>
<p>

<form action="LicenseProcess.php" method="post">

MAC Address is: ';

$message .= $_GET['UI'];

$message .= '
<BR>
 <input type="text" name="MACAddress" value=
 ';

$message .= $_GET['UI'];
$message .= '
 >

 <p>Product: iSplice</p>
<table>
<tr>
    <td align=right>* first name:</td>
    <td><input type="text" name="first_name" size=40 /></td>
</td>
<tr>
    <td align=right>* last name:</td>
    <td><input type="text" name="last_name" size=40 /></td>
</tr>
<tr>
    <td align=right>* company:</td>
    <td><input type="text" name="company" size=40 /></td>
</tr>
<tr>
    <td align=right>contact number:</td>
    <td><input type="text" name="contact_number" size=40 /></td>
</tr>
<tr>
    <td align=right>* country:</td>
    <td><input type="text" name="country" size=40 /></td>
</tr>
<tr>
    <td align=right>* email address:</td>
    <td><input type="text" name="email_address_1" size=40 /></td>
</tr>
<tr>
    <td align=right>* email address (again):</td>
    <td><input type="text" name="email_address_2" size=40 /></td>
</tr>
<tr>
    <td align=right></td>
    <td><input type="checkbox" name="annodex_emails" size=40 />Please email me update information related to Annodex/iSplice</td>
</tr>
<tr></tr>
';

 if ($_POST[SourceInput] != 'iSpliceAdministrator')
      {
          $message .= '
          <tr>
           <td></td>
           <td><input type="submit" Value="Get 30 day Trial License" /></td>
          </tr>
          <tr>
           <td><input type="hidden" align=right value=30 name="number_of_days" /> </td>
           <td><input type="hidden" align=right value=50 name="number_of_runs" /></td>
           <td><input type="hidden" value="false" name="admin_request"/></td>
          </tr>
          ';
      }
      else
      {
        $message .= '
        <tr height=20>
        </tr>
        <tr>
         <td align=right style="color:red">* number of days (0=full license)</td>
         <td><input type="text" align=right value=30 name="number_of_days" size=5 style="color:red;background-color:#ffcccc"/></td>
        </tr>
        <tr>
         <td align=right style="color:red">* number of runs (0=full license)</td>
         <td><input type="text" align=right value=50 name="number_of_runs" size=5 style="color:red;background-color:#ffcccc"/></td>
        </tr>
        <tr>
         <td></td>
         <td><input type="submit" Value="Get Special License"/></td>
        </tr>
        <tr>
         <td><input type="hidden" value="true" name="admin_request"/></td>
        </tr>
        ';
      }

$message .= '
</table>

</form>

Contact us: <a href="mailto:administrator@annodex.net">iSplice Administrator</a>

<BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR><BR>
';

$sht->html_middle($message);

#do left column

$sht->html_left(null);

# right column

$sht->html_right(null);

#footer

$sht->html_footer();

?>