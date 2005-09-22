<? include "header.php"; ?>
<h2>Info</h2>
<div class="roundcont">
<div class="roundtop">
<img alt="" src="images/corner_topleft.jpg" class="corner" style="display: none" />
</div>
<div class="newscontent">

<h3>About</h3>
<p>
The Icecast Authentication Manager was written as a demo app that can be used in conjunction with
icecast's URL-based authenticator.  It was written to showcase the kind of thing that can be done
with the authenticator, but is by no means intended to be a complete application.  It's all written
in PHP and uses mySQL as a database store.  If you would like to extend it or expand on the features
please share those with the community at large.
</p>
<p>
<font color=yellow>
- oddsock
	<br>
	<br>
<h3>README</h3>
<pre>
<p>
<? $readme = file_get_contents("README");
   $readme = str_replace("<", "&lt;", $readme);
   $readme = str_replace(">", "&gt;", $readme);
   print $readme;
 ?>
</p>
</pre>
	<br>
</p>
</div>
<div class="roundbottom">
<img alt="" src="images/corner_bottomleft.jpg" class="corner" style="display: none" />
</div>	
</div>
<? include "footer.php"; ?>
