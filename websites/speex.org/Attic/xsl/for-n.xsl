<?xml version='1.0'?>
<xsl:stylesheet version='1.0'
	xmlns:xsl='http://www.w3.org/1999/XSL/Transform'
	xmlns='http://www.w3.org/1999/xhtml'>

<!-- for-n:
     Like <xsl:for-each/>, but it relies on matches rather than having
     templates inline.
     Use it like this:
     ...
     <xsl:call-template name='for-n'>
        <xsl:with-param name='elements' select='//item'/>
        <xsl:with-param name='n' select='3'/>
     </xsl:call-template>
     ...
-->
<xsl:template name='for-n'>
    <xsl:param name='elements'/>
    <xsl:param name='n'/>
    <xsl:if test='$elements and $n > 0'>
        <xsl:apply-templates select='$elements[1]'/>
        <xsl:call-template name='for-n'>
            <xsl:with-param name='elements' select='$elements[position() != 1]'/>
            <xsl:with-param name='n' select='$n - 1'/>
        </xsl:call-template>
    </xsl:if>
</xsl:template>
</xsl:stylesheet>
