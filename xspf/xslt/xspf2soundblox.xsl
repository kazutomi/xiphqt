<!--
	Convert XSPF to Soundblox data format.
	Author: Lucas Gonze <lucas@gonze.com>
	Copyright 2004 Lucas Gonze.
	Licensed under version 2.0 of the Gnu General Public License.

	A proof of concept, not robust yet.  Needs work by someone more
	familiar with Soundblox than I am.
-->
<xsl:stylesheet version = '1.0'
				xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
				xmlns:xspf="http://xspf.org/ns/0/">

  <xsl:output method="xml" indent="yes"/>

  <xsl:template match="/">

	<soundbloxdata>			  
      <customize autostart="false" startopen="true"/>
	  <playlist>

		<xsl:if test="string(//xspf:title)">
		  <xsl:attribute name="name">
			<xsl:value-of select="//xspf:title"/>
		  </xsl:attribute>
		</xsl:if>	  

		<xsl:apply-templates select="*"/>

	  </playlist>
	</soundbloxdata>

  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track">
	<track>
	  <xsl:apply-templates select="*"/>
	</track>
  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track/xspf:location">
	<xsl:if test="string(.)">
	  <!-- fixme: test extension to confirm it is an mp3 -->
	  <mp3url><xsl:value-of select="."/></mp3url>
	  <downloadurl><xsl:value-of select="."/></downloadurl>
	</xsl:if>
  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track/xspf:image">
	<xsl:if test="string(.)">
	  <imageurl><xsl:value-of select="."/></imageurl>
	</xsl:if>
  </xsl:template>

  <xsl:template match="//xspf:trackList/xspf:track/xspf:annotation">
	<xsl:if test="string(.)">
	  <comments><xsl:value-of select="."/></comments>
	</xsl:if>
  </xsl:template>

  <!-- Suppress items not accounted for in templates.  Disable during
	debugging, enable in production.  -->
  <xsl:template match="text()"/>

</xsl:stylesheet>
