<xsl:stylesheet version='1.0'
	xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<xsl:output
	method="xml"
	indent='yes'
	encoding='UTF-8'/>

<xsl:template match="/">
<html>
    <head><title>News</title></head>
<body>
    <xsl:for-each select='//item'>
        <div class='newsitem'>
            <div class='newsheader'>
                <div class='title'><xsl:apply-templates select='title'/></div>
                 
                <div class='meta'><xsl:apply-templates select='when'/></div>
            </div>
            <div class='newscontent'>
                    <xsl:copy-of select="@*"/>
            </div>
        </div>
    </xsl:for-each>
</body>
</html>
</xsl:template>

</xsl:stylesheet>
