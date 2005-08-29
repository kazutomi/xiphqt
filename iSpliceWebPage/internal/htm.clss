<?php
# This class provdes methods
# for creating html pages

class htm
 {  

 
   #function for printing html header tags 
   function html_begin($title, $anyscript)
    {

$message = '
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
    "http://www.w3.org/TR/html4/strict.dtd">

<html lang="en">
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
        <style type="text/css"><!--
            @import "skidoo_too.css";
        --></style>
        <link rel="stylesheet" href="./skidoo_too_print.css" type="text/css" media="print">
        <script type="text/javascript" src="./javascript/ruthsarian_utilities.js"></script>
        <script type="text/javascript">
';

$message .= "
        <!--
            var font_sizes = new Array( 100, 110, 120 ); 
            var current_font_size = 0;
            if ( ( typeof( NN_reloadPage ) ).toLowerCase() != 'undefined' ) { NN_reloadPage( true ); }
            if ( ( typeof( opacity_init  ) ).toLowerCase() != 'undefined' ) { opacity_init(); }
            if ( ( typeof( set_min_width ) ).toLowerCase() != 'undefined' ) { set_min_width( 'pageWrapper' , 600 ); }
            if ( ( typeof( loadFontSize ) ).toLowerCase() != 'undefined' ) { event_attach( 'onload' , loadFontSize ); }
        //-->
        </script>
";

        echo $message;

          if ($title)
          {
             print ("<title>". $title ."</title>\n");
          }

          if ($anyscript)
          {
             print ("$anyscript");
          }

        print("</head>\n");
     }

#function for printing the html top menu items
#uses default if $toplist is null
function html_top($header, $toplist)
{
  if ($toplist == null)
  {
    # use default
    $toplist = '
        <ul
         ><li class="hide"
            ><a class="hide" href="../../">Hidden</a
            ><span class="divider"> : </span
                ></li
        ><li
            ><a href=".?UI='.$_GET['UI'].'">Home</a
            ><span class="divider"> : </span
            ></li
        ><li
            ><a href="./downloads.php?UI='.$_GET['UI'].'">Download</a
            ><span class="divider"> : </span
            ></li
        ><li
            ><a href="./LicenseAgreement.php?UI='.$_GET['UI'].'">Licenses</a
            ><span class="divider"> : </span
            ></li
        ><li
            ><a href="./iSpliceScreens.php?UI='.$_GET['UI'].'">Screen Shots</a
            ><span class="divider"> : </span
        ></li

        ></ul>
        ';
  }

  $message = '
    <body>
        <div id="pageWrapper">
            <div id="masthead" class="inside">

<!-- masthead content begin -->
     '.$header.'

<!-- masthead content end -->

                <hr class="hide">
            </div>
            <div class="hnav">

<!--
    you must preserve the (lack of) whitespace between elements when creating 
    your own horizontal navigation elements, otherwise the extra whitespace 
    will break the visual layout. although how it breaks (tiny spaces between 
    each element) is an effect that some may desire.
-->
    '.$toplist.'
             <hr class="hide">
        </div>
  ';

  echo $message;
}

#function for printing the html top middle section
function html_middle($middle)
{
  $message = '
      <div id="outerColumnContainer">
           <div id="innerColumnContainer">
                <div id="SOWrap">
                     <div id="middleColumn">
                          <div class="inside">
                          '.$middle.'
                           <hr class="hide">
                      </div>
                </div>
  ';

  echo $message;
}


#function for printing the html left menu items
function html_left($leftlist)
{
  # put start in
  $message = '
      <div id="leftColumn">
          <div class="inside">
              <div class="vnav">
              '.$leftlist.'
              </div>
       ';

  # only include resizing if there are items in the left column
  #if ($leftlist == "")
  {
     $message .= "
              <script type=\"text/javascript\">
              <!--
                  var browser = new browser_detect();
                  if ( browser.versionMajor > 4 || !( browser.isIE || browser.isNS ) )
                  {
                      /* only offer style/font changing to version 5 and later browsers
                       * which have javascript enabled. curiously, if you print this out
                       * in NS4, NS4 breaks for some reason.
                       */
                       document.write('                                    \
                               <p class=\"fontsize-set\">                            \
                               <a href=\"#\" onclick=\"setFontSize(0); return false;\"         \
                               ><img src=\"images/font_small.gif\" width=\"17\" height=\"21\"    \
                               alt=\"Small Font\" title=\"Small Font\"         \
                               /><\/a>                                 \
                               <a href=\"#\" onclick=\"setFontSize(1); return false;\"         \
                               ><img src=\"images/font_medium.gif\" width=\"17\" height=\"21\"   \
                               alt=\"Medium Font\" title=\"Medium Font\"           \
                               /><\/a>                                 \
                               <a href=\"#\" onclick=\"setFontSize(2); return false;\"         \
                               ><img src=\"images/font_large.gif\" width=\"17\" height=\"21\"    \
                               alt=\"Large Font\" title=\"Large Font\"         \
                               /><\/a>                                 \
                               <\/p>                                       \
                               ');
                   }
                }
            //-->
            </script>
            ";
   }

   # And finish off
   $message .= '
            <hr class="hide">
         </div>
      </div>
      <div class="clear">
      </div>
   </div>
   ';

   echo $message;
}


#function for printing html right
function html_right($rightlist)
{
  $message = '
       <div id="rightColumn">
           <div class="inside">
               '.$rightlist.'
               <hr class="hide">
           </div>
       </div>
    ';

    echo $message;
}


#function for printing html footer
function html_footer()
{
  $message = '
                    <div class="clear"></div>
                </div>
            </div>

            <div id="footer" class="inside">

    <!-- footer begin -->

    <p style="margin:0;">
       &copy; CSIRO 2005.<br>
    </p>

    <!-- footer end -->

                <hr class="hide">
            </div>
        </div>
    </body>
</html>
';
  echo $message;
}

}

?>