<?xml version='1.0'?>
<xsl:stylesheet version='1.0'
	xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<!-- roughly, the identity function for XHTML elements.
     This list was cribbed from Mozilla's sidebar; blame them if anything's
     missing.
-->

<xsl:template match='shortnews'>
    <div id='headlines'>
        <!-- Kay says this is the way to bring in info from another
             document, so this is what I'm doing.... -->
        <xsl:for-each select='document("../news.xml")'> 
            <xsl:call-template name='for-n'>
                <xsl:with-param name='elements' select='//item'/>
                <xsl:with-param name='n' select='5'/>
            </xsl:call-template>
        </xsl:for-each>
        <div class='readmorenews'>
            <a href='news.html'>Read More News…</a>
        </div>
    </div>
</xsl:template>

<xsl:template match='longnews'>
    <!-- this one doesn't have the headlines div so it can be distinguished
         from the one on news.html -->
    <xsl:for-each select='document("../news.xml")'>
        <xsl:for-each select='//item'>
            <xsl:apply-templates select='.'/>
        </xsl:for-each>
    </xsl:for-each>
</xsl:template>

<xsl:template match='item'>
    <div class='newsitem'>
        <div class='hlheader'>
            <div class='title'><xsl:apply-templates select='title'/></div>
             
            <div class='meta'><xsl:apply-templates select='when'/></div>
        </div>
        <div class='hlcontent'>
            <!-- I don't understand why this works and nothing else does,
                 but the damn thing works.
                 Problems with other attempts:
                 - outputs <item>
                 - doesn't output  even though 
                 -->
            
            <xsl:for-each select='what'>
                <xsl:apply-templates select='.'/>
            </xsl:for-each>
        </div>
    </div>
</xsl:template>


</xsl:stylesheet>
