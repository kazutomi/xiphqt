<?xml version='1.0'?>
<xsl:stylesheet version='1.0'
	xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
	xmlns='http://www.w3.org/1999/xhtml'>
<xsl:import href='xsl/for-n.xsl'/>
<xsl:import href='xsl/xhtml.xsl'/>
<xsl:import href='xsl/news.xsl'/>
<xsl:output
	method="xml"
	indent='yes'
	doctype-public='-//W3C//DTD XHTML 1.0 Strict//EN'
	doctype-system='http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd'
	encoding='UTF-8'/>

<xsl:template match="/">
<html xmlns='http://www.w3.org/1999/xhtml' lang='en-US' xml:lang='en-US'>
	<head>
	    <meta http-equiv='content-type' content='text/html; charset=utf-8'/>
		<style type='text/css' media='all' title='Somewhat Classy'>
			@import url(style/all.css);
		</style>
		<title>
            <xsl:value-of select='//html/head/title'/> :: Speex :: Xiph.org Foundation
        </title>
        <link rel='start'		href='http://www.speex.org/' />
		<link rel='help'		href='http://www.speex.org/faq/' />
        <link rel='copyright'	href='#footer' />
	</head>
	<body>
        <div class='skiplinks'><a href='#content' accesskey='2'>Skip to main content</a></div>
		<xsl:call-template name='render-banner'/>
		<xsl:call-template name='render-nav'/>
		<div class='main'>
			<div id='content'>
				<xsl:apply-templates select='//html/body/*'/>
			</div>
			<xsl:call-template name='render-footer'/>
		</div>
	</body>
</html>
</xsl:template>

<xsl:template name='render-banner'>
	<div id='banner'>
		<!-- CDATA escaping needed here and at the bottom of the div because
			1) if I do a 'real' empty div, the xsl processor will/can
			   minimize it
			1a) This minimization causes IE and Moz to soil themselves -
				unminimized spacer divs are fine, violating 
				the <x/>==<x></x> identity

			oh, this broke on the Mac version of xsltproc.

			-->
		<span class='left'>
			<a href='http://www.xiph.org/'>xiph.org</a> &#x00b7;
			<a href='http://www.vorbis.com/'>vorbis.com</a> &#x00b7;
			<a href='http://www.icecast.org/'>icecast.org</a> &#x00b7;
			<a href='http://www.speex.org/' accesskey='1'>speex.org</a>
		</span>
		<span class='right'>Open Standards for Internet Multimedia</span>
		<xsl:text><![CDATA[<div class='spacer'> </div>]]></xsl:text>
	</div>
</xsl:template>


<xsl:template name='render-nav'>
	<div class='nav'>
		<ul>
			<li class='first'><a href='index.html'
								 title='Return to the speex.org homepage.'>Home</a></li>
			<li><a href='news.html' title='Catch up on old news'>All News</a></li>
			<li><a href='comparison.html' title='See a feature list'>Comparison</a></li>
			<li><a href='audio/samples/' title='Listen to Speex samples'>Samples</a></li>
			<li><a href='download.html' title='Download Speex'>Download</a></li>
			<li><a href='docs.html'
				   title='View the available documentation'>Docs &amp;
				   <abbr title='Frequently Asked Questions'>FAQ</abbr></a></li>
			<li><a href='todo.html' title='See what’s in the pipe'>Roadmap</a></li>
			<li><a href='projects.html'
				   title='See what others are doing with Speex'>Plugins &amp; Software</a></li>
			<li><a href='contact.html' title='Get in touch with us'>Contact</a></li>
		</ul>
	</div>
</xsl:template>

<xsl:template name='render-footer'>
	<div id='footer'>
		<img class='separator' alt='' src='images/hfuzz.png' width='100%' height='1'/>
			<a href='http://ukcdr.org/issues/cd/quick/'
			 ><img src='images/badcd002.png'
				   alt='say NO to corrupt audio discs'
				   class='corruptaudio'/></a>
		<p id='legalese'>
			<span class='hidden'>
				<a href='http://www.xiph.org/ogg/'
					>OggSQUISH</a>,
				<a href='http://www.vorbis.com/'>Ogg Vorbis</a>,
				<a href='http://www.xiph.org/'>Xiph.Org</a>,
				the <a href='http://www.xiph.org/xiphname.html'>Xiph.Org Fish Logo</a>,
				the <a href='http://www.xiph.org/xiphname.html'
					>Thor-and-the-Snake logo</a>,
				and the <a href='http://www.xiph.org/xiphname.html'
					>Laser-Playback-Head-of-Omniscience logo</a>
				are trademarks of Xiph.Org.
			</span>
			These pages 
			&#x00a9; 2003 Xiph.Org Foundation. All rights reserved.
		</p>
		<p>
			<a href='contact.html'>Comments and questions</a>
			about this website are welcome; we use valid
			<a href='http://validator.w3.org/check/referer'
			   title='Check the validity of this page’s XHTML here'
			   ><abbr title='eXtensible Hypertext Markup Language'>XHTML</abbr></a> and
			<a href='http://jigsaw.w3.org/check/referer'
			   title='Check the validity of this page’s CSS here'
			   ><abbr title='Cascading Style Sheets'>CSS</abbr></a>2.
			
		</p>
		<p class='ahem'>
			This site will look much better in a browser with decent
			<a href="http://www.webstandards.org/act/campaign/buc/"  
			   title="Download a browser that complies with Web standards."
			   ><abbr title='Cascading Style Sheets'>CSS</abbr> support</a>, 
			but its content is accessible to any browser or Internet device.
		</p>
	</div>
</xsl:template>
</xsl:stylesheet>
