<?xml version='1.0'?>
<xsl:stylesheet version='1.0'
	xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
<!-- roughly, the identity function for XHTML elements.
     This list was cribbed from Mozilla's sidebar; blame them if anything's
     missing.
-->
<xsl:template match=' a | abbr | acronym | address | area | b | base | bdo
					| big | blockquote
					| body | br | button | caption | cite | code | col
					| colgroup | dd
					| del
					| dfn | div | dl | dt | em | fieldset | form
					| h1 | h2 | h3 | h4 | h5 | h6 | head | hr | html | i | img
					| input
					| ins | kbd | label | legend | li | link | map | meta
					| noscript
					| object | ol | optgroup | option | p | param | pre | q
					| samp
					| script
					| select | small | span | strong | style | sub | sup
					| table | tbody
					| td | textarea | tfoot | th | thead | tr | tt
					| ul | var '>
					
  <xsl:copy>
    <xsl:copy-of select="@*"/>
    <xsl:apply-templates/>
  </xsl:copy>
</xsl:template>
</xsl:stylesheet>
