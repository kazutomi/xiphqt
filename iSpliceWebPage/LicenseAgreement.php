<?php

include( "htm.clss");

#create objects
$sht      = new htm();
$message = "iSplice Annodex Creator";
$sht->html_begin($message,null);

$header = "<h1>iSplice: The Annodex Creator</h1>";

$sht->html_top($header, null);

#<!--- middle (main content) column begin -->


$message = '
        <h2>CSIRO COMMERCIAL SOFTWARE LICENCE</h2>

        <h2>FOR THE SOFTWARE KNOWN AS "iSplice"</h2>

<p><br><b>1. Meaning of Words</b><b></b>
<p><b>\'CSIRO\'</b> means the Commonwealth Scientific and Industrial Research
Organisation of Limestone Avenue, Campbell, Australian Capital Territory,
Australia acting through its Division of Mathematical and Information Sciences,
Building E6B, Macquarie University Campus, Herring Road, North Ryde NSW 2113,
Australia
<br>&nbsp;
<br><b>\'Fee\'</b> means the amount of upfront payment in Australian dollars
that CSIRO charges for providing the Software as published on the Software
website at <a href="http://www.aidabrowser.com/iSplice">http://www.aidabrowser.com/iSplice</a><a href="http://www.aidabrowser.com/iSplice"></a>

<p><b>\'Intellectual Property\'</b> means any copyright work or other work
(including any work or item created in the future), patentable invention,
design, circuit layout, new plant variety, trademark, know-how or confidential
information and any other intellectual property defined in Article 2 of
the Convention establishing the the world Intellectual Property Organisation
of July 1967.
<p><b>\'Licence\'</b> means this licence agreement between You and CSIRO
comprising (a) the terms that You accept by selecting the "I Agree" icon
and submitting the Registration to CSIRO; and (b) the Purchase Order Form
if used.&nbsp; This licence agreement covers both the single server network
and multiple platform uses.
<p><b>\'Purchase Order Form\'</b> means the iSplice Software order form available
at the Software website at <a href="http://www.aidabrowser.com/iSplice">http://www.aidabrowser.com/iSplice</a>
which You are to complete, sign and return to CSIRO for a manual purchase
order.
<p><b>\'Payment Arrangements\'</b> means the method by which You wish to
pay for the Software and which is either (a) the online purchase method
using the secure payment gateway at the Software website or (b) the payment
method You are to nominate when You return the signed Purchase Order Form
to CSIRO.

<p><b>\'Software\'</b> means the Annodex Creation package known
as&nbsp; <b>"iSplice"</b>, comprising executable files, dynamic link libraries and
documentation, implemented as a package for the multimedia
environment.
<p><b>\'Registration\'</b> means the registration with CSIRO which You submit
to CSIRO by providing the information and details requested on the Software
website at <a href="http://www.aidabrowser.com/iSplice">http://www.aidabrowser.com/iSplice</a>
<p><b>\'You\'</b> means you the licensee and any permitted user(s) of the
Software.

<p><b>2.&nbsp; The Licence CSIRO grants to You</b>
<p>CSIRO grants You a non-exclusive, non-transferable, perpetual licence
to use the Software in the Field for your research and/or business purposes
in accordance with the terms of this Licence only.  
<p>CSIRO reserves the right to refuse to licence the Software to any person
without giving reasons thereof.&nbsp; If CSIRO does not approve the Registration
or Purchase Order Form submitted by You, your sole remedy will be the refund
of any Fee paid to CSIRO.
<p>If CSIRO accepts your Registration and Purchase Order Form under these
Licence terms, CSIRO will provide the Software as soon as practicable by
sending You an email to your specified email address (or as otherwise nominated
by You) to allow download of the Software for your selected platform.
<br>&nbsp;
<br>This Licence is formed when You have clicked the "I Agree" icon, submitted
the Registration and Purchase Order Form and CSIRO approves by validating
your Registration.
<p><b>3. What you must do to submit the Registration and Purchase Order
Form</b>
<p>You must provide and ensure the accuracy and completeness of your Registration
and Purchase Order Form.
<p>You must select the "I Agree" icon at the end of this Licence to indicate
your agreement to the terms of the Licence.&nbsp; You must submit the Registration
after You have selected the "I Agree" button to register for the provision
of Software.&nbsp; You must complete and return the Purchase Order Form
to CSIRO.

<p>You agree that CSIRO may retain all Registration and Purchase Order
Form data provided by You for its record-keeping purposes and You must
read CSIRO\'s privacy statement and policy on the use of its websites which
is made available to You at the Software website.
<p>&nbsp;If You are employed and intend using the Software in connection
with your employment duties then You should only accept this Licence if
your employer has authorised You to do so on its behalf.&nbsp; By accepting
this Licence You are warranting to CSIRO that You are authorised to do
so on behalf of your employer.
<p><b>4. Use of names</b>
<p>You must not without CSIRO\'s written permission use the name, any trademark
or logo of CSIRO to claim any sponsorship, endorsement, approval or affiliation
or other association with CSIRO by virtue of this Licence.
<p><b>5. Proprietary rights</b>
<p>Title, ownership and Intellectual Property rights in the Software shall
remain with CSIRO.&nbsp; Nothing in this Licence transfers to You ownership
of Intellectual rights in the Software.&nbsp; You must not remove or alter
any logo, copyright or other proprietary notices, symbols or labels in
the Software.
<p><b>6. Fee and Payment</b>
<p>You must pay CSIRO the Fee in accordance with the Payment Arrangements.&nbsp;

You should note that the Fee is in Australian dollars and does not cover
any upgrades or new versions of the Software.
<p><b>7. Your rights to use the Software</b>
<p>You may use the Software for the Purpose strictly in accordance with
the terms of the Licence only.&nbsp; You may make copies of the Software
only as reasonably required for backup purposes.
<p>You must not:
<br>(a) reverse engineer, decompile or disassemble the Software;
<br>(b) adapt, extend, enhance or make any other improvements to the Software
without CSIRO\'s prior written consent;
<br>(c) distribute, sell, sublicense or otherwise make the whole or part
of the Software available for use by a third party;
<br>(d) allow the Software to be combined with or incorporated into other
software; or
<br>(e) release the Software on the Internet or any other public communication
network.
<br>&nbsp;
<p><b>8.&nbsp; Conditions on multiple simultaneous users</b>

<p>For multiple platform use of the Software, You undertake to ensure that
the permitted users are aware of the terms of this Licence prior to use
and comply with all the obligations imposed by this Licence.&nbsp; Permitted
users must be the licensee\'s employees, contractors, agents or students
only and You remain at all times responsible for their actions or omissions
in relation to use of the Software.
<p>You must not allow more than the permitted number of users covered by
the Fee to use the Software at any one time during the term of this Licence.
<p>If CSIRO requests in writing, You must provide CSIRO with:
<br>(a) relevant information and records for the sole purpose of monitoring
your compliance with this Licence; or
<br>(b) a complete record to date of the permitted users to whom You have
allow access and use of the Software.
<p><b>9. Software Support</b>
<p>Software support will be your responsibility and CSIRO has no obligation
to provide to You any bug fix support services in relation to the Software.&nbsp;
However, CSIRO will reasonably respond at its absolute discretion during
business hours to incident reports emailed by You to&nbsp; admin@www.annodex.net
<p>Where CSIRO addresses a defect or provides a new feature through the
release of a new version of the Software, You may make separate arrangements
to purchase the new version.
<p><b>10. CSIRO limits its liability</b>

<p>You are responsible for determining that the Software is suitable for
your own use or purpose.
<p>You assume all risk for any loss or damage resulting directly or indirectly
from your use of or inability to use the Software.
<p>TO THE EXTENT PERMITTED BY LAW, CSIRO EXCLUDES ALL LIABILITY FOR ANY
LOSS OR DAMAGE (INCLUDING DIRECT, INDIRECT, SPECIAL OR CONSEQUENTIAL LOSS
OR DAMAGE) ARISING FROM YOUR USE OF THE SOFTWARE OR OTHERWISE IN CONNECTION
WITH THIS LICENCE.
<p>WHERE ANY STATUTE OR LAW IMPLIES WARRANTIES OR CONDITIONS INTO THIS
LICENCE, WHICH CANNOT BE LAWFULLY MODIFIED OR EXCLUDED UNDER THIS LICENCE
(\'NON-EXCLUDABLE CONDITION\') THEN THIS LICENCE WILL BE READ SUBJECT TO
SUCH NON-EXCLUDABLE CONDITION.&nbsp; WHERE SUCH STATUTE OR LAW PERMITS,
CSIRO LIMITS ITS LIABILITY TO YOU FOR BREACH OF SUCH NON-EXCLUDABLE CONDITION
AT ITS OPTION TO:
<p>(I) RE-SUPPLYING THE SOFTWARE; OR
<br>(II) REFUNDING THE AMOUNTS PAID BY YOU FOR THE SOFTWARE.
<p><b>11. CSIRO makes no representations</b>
<p>CSIRO warrants that for 30 days following the date of supply of the
Software, it will operate substantially in accordance with the Software
documentation.&nbsp; If during this warranty period the Software fails
to operate substantially in accordance with the Software documentation,
CSIRO will supply another copy of the Software at its cost.
<p>Except as expressly warranted above, CSIRO does not warrant or make
any representations:
<br>(a) that the Software is of merchantable quality, suitable for your
use, or is fit for any other purpose;
<br>(b) that operation of the Software will be-uninterrupted or that the
Software is error-free;
<br>(c) regarding the results of any use of the whole or any part of the
Software;

<br>(d) as to the accuracy, reliability or content of any data, information,
service or goods obtained through any use of the whole or any part of the
Software;
<br>(e) as to the accuracy, reliability or content of any Internet sites
addressed by the URLs in the database forming part of the Software; or
<br>(f) that the use of the Software will not infringe the intellectual
property rights of a third party.
<p><b>12. The end of the Licence</b>
<p>This Licence ends if You and CSIRO by mutual agreement in writing end
it.
<p>CSIRO may terminate this Licence immediately by giving You notice if
You breach any provision of this Licence and fail to remedy the breach
within 30 days of CSIRO requiring You to do so.
<p>CSIRO may also terminate this Licence immediately by giving You written
notice if You attempt to assign any of the rights arising out of this Licence
to a third party or if You are subject to any form of insolvency administration.
<p>On termination, all your rights to use the Software cease and You must
destroy all copies of the Software in your possession.
<p><b>13. Resolving disputes</b>
<p>In any dispute arising out of or in connection with this Licence, You
agree to first negotiate in good faith with a senior CSIRO officer to resolve
it.&nbsp; If the dispute is not resolved by those negotiations within 30
days, You agree that the matter may be referred to the Australian Commercial
Dispute Centre Limited (\'ACDC\') for resolution by mediation and if necessary
by arbitration in accordance with the Conciliation Rules of the ACDC.
<p><b>14. The governing law</b>
<p>This Licence is governed by and construed in accordance with the laws
of New South Wales, Australia.

<p><b>15. The entire licence</b>
<p>This Licence overrides the provisions of any purchase order, invoice
or other documentation that You may issue in relation to the Software.
<p>
';

 $UI = $_GET['UI'];
 $location = "iSpliceLicense.php?UI=$UI";

 $message .= '
 <br><br>
<!--    <a href="iSplicelicence_terms_5Aug2005.pdf"><b>View printable Licence</b></a> in PDF format -->
    </p> 
    <form>     
            <p>  By pressing the accept button the user acknowledges that  they<br> 
              have read and agree to the terms and conditions of the licence agreement. 
            </p> 
            <p>  
             <input type="button" value="I Accept" name="accept" onclick="location = \''.$location.'\';">
             <input type="button" value="Reject" name="reject" onclick="location = \'index.html\';">
            </p>  
        </form>             
 
   </p> </p>
   ';

$message .= '
   <p align="center"><font size="2">
   <b> Last updated: </b>August 05, 2005.<br><br> Copyright 2005, CSIRO Australia
   <br>Use of this web site and information available from<br>
   it is subject to our
   <a href="http://www.csiro.au/legalnotices/disclaimer.html">
   <font size="2">Legal Notice and Disclaimer</font> </a>
   and <a href="http://www.csiro.au/index.asp?type=aboutCSIRO&amp;xml=privacy&amp;stylesheet=generic">
   <font size="2">Privacy Statement</font></a></p></font>
';

$sht->html_middle($message);

#do left column

$sht->html_left(null);

# right column

$sht->html_right(null);

#footer

$sht->html_footer();

?>