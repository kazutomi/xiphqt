<?xml version='1.0'?>
<xsl:stylesheet version='1.0'
	xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:import href='xsl/for-n.xsl'/>
<xsl:import href='xsl/xhtml.xsl'/>
<xsl:output
	method="xml"
	indent='yes'
	encoding='UTF-8'/>

<xsl:template match="/">
<html>
    <title>Home Page</title>
<body>
<div id='h'>Speex</div>
<h1>About</h1>
<p>
    The <a href="http://www.speex.org/">Speex project</a>      
    aims to build a patent-free,
    <a href='fsos.html'>Open Source/Free Software</a>
    voice codec.  Unlike other codecs like MP3 and
    <a href="http://www.vorbis.com/">Ogg Vorbis</a>,
    Speex is designed to compress voice at low bitrates
    in the 8-32 <abbr title='kilobits per second'>kbps</abbr>/channel range.
    Possible applications include
    <abbr title='Voice over IP'>VoIP</abbr>, internet audio streaming,
    archiving of speech data (e.g. voice mail), and audio books. In
    some sense, it is meant to be complementary to the Ogg Vorbis codec.
</p>
<p>Speex is:</p>
<ul>
    <li><a href='fsos.html'>Open Source and Free Software</a>,
        released under the Xiph.Org variant of the BSD license</li>
    <li>Patent and royalty free</li>
    <li>Optimized for speech compression</li>
</ul>
<p>In addition, Speex features:</p>
<ul>
    <li>Both narrowband (8 kHz) and wideband (16 kHz) compression</li>
    <li>Many available bitrates ranging from 6 kbps to 42 kbps</li>
    <li>Variable Bit-Rate operation (VBR)</li>
    <li>Voice Activity Detection (VAD, integrated with VBR)</li>
</ul>
<p> 
    If you are interested in participating
    in the project, contact the Project Lead,
    <a href='http://www.xiph.org/~jm'>Jean-Marc Valin</a>, or
    <a href="vmail.html">join our mailing list</a>. 
    At the moment, we are seeking developers with signal processing
    and speech coding knowledge, people with knowledge about patents,
    and people who can help integrate Speex with existing standards.
</p>

<p>
    Currently, <a href='http://www.linphone.org/'>LinPhone</a>,
    <a href='http://www.openh323.org'>OpenH323</a>, and
    <a href='http://www.gnomemeeting.org'>GnomeMeeting</a> are some of the
    projects currently using Speex. For a complete list of projects with
    both current and planned Speex support, visit the
    <a href='projects.html'>Projects</a> page.
</p>

<h1>Headlines</h1>
<div id='headlines'>
    <xsl:call-template name='for-n'>
        <xsl:with-param name='elements' select='//item'/>
        <xsl:with-param name='n' select='5'/>
    </xsl:call-template>
    <div class='readmorenews'>
        <a href='news.html'>Read More News&#x2026;</a>
    </div>
</div>
</body>
</html>
</xsl:template>

<xsl:template match='item'>
    <div class='newsitem'>
        <div class='hlheader'>
            <div class='title'><xsl:apply-templates select='title'/></div>
             
            <div class='meta'><xsl:apply-templates select='when'/></div>
        </div>
        <div class='hlcontent'>
            <xsl:copy>
                <xsl:copy-of select="@*"/>
                <xsl:apply-templates select='what'/>
            </xsl:copy>
        </div>
    </div>
</xsl:template>

</xsl:stylesheet>
