<!--
	Convert XSPF to SMIL.
	Author: Lucas Gonze <lucas@gonze.com>

	Copyright 2004 Lucas Gonze.  Licensed under version 2.0 of the Gnu
	General Public License.
-->
<xsl:stylesheet version = '1.0'
				xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
				xmlns:xspf="http://xspf.org/ns/0/"
				>

  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">
	<smil>
	  <body>
		<seq>		  
		  <xsl:apply-templates select="*" />
		</seq>
	  </body>
	</smil>
  </xsl:template>

  <xsl:template match="//xspf:annotation" ></xsl:template>
  <xsl:template match="//xspf:creator" ></xsl:template>
  <xsl:template match="//xspf:info" ></xsl:template>
  <xsl:template match="//xspf:location" ></xsl:template>
  <xsl:template match="//xspf:identifier" ></xsl:template>
  <xsl:template match="//xspf:image" ></xsl:template>
  <xsl:template match="//xspf:link" ></xsl:template>
  <xsl:template match="//xspf:meta" ></xsl:template>
  <xsl:template match="//xspf:date" ></xsl:template>
  <xsl:template match="//xspf:attribution" ></xsl:template>
  <xsl:template match="//xspf:license" ></xsl:template>

  <xsl:template match="//xspf:trackList">
	  <xsl:apply-templates select="*" />
  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track">
	<xsl:apply-templates select="xspf:location"/>
  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track/xspf:location" >
	<audio>
	  <xsl:attribute name="src">
		<xsl:value-of select="." />
	  </xsl:attribute> 

	  <xsl:if test="string(../xspf:annotation)">
		<xsl:attribute name="title">
		  <xsl:value-of select="../xspf:annotation"/>
		</xsl:attribute> 
	  </xsl:if>

	</audio>
  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track/xspf:image" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:annotation" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:creator" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:title" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:album" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:trackNum" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:duration" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:identifier" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:info" ></xsl:template>
  <xsl:template match="//xspf:trackList/xspf:track/xspf:meta" ></xsl:template>

  <!-- Suppress items not accounted for in templates.  Disable during debugging, enable in production.
  -->
  <xsl:template match="text()" />

</xsl:stylesheet>

