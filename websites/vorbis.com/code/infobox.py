# infobox.py

class InfoBox:
    def __init__(self):
        self.title = 'no title'
        self.more = None
        self.show_bottom_more = 0

        self.borderclass = 'defaultborder'
        self.titleclass = 'defaulttitle'
        self.contentclass = 'defaultcontent'

        self.title_color = "#000000"
        self.title_image = None
        self.title_image_r = None

        self.fontface = "Verdana, Arial, Helvetica, Sans-Serif"
        self.fontsize = -1

    def start(self):
        if self.title_image:
            titleimg = ' background="%s"' % (self.title_image,)
        else:
            titleimg = ''
        if self.title_image_r:
            titleimg_r = ' background="%s"' % (self.title_image_r,)
        else:
            titleimg_r = ''

        one = """
        <table border="0" width="100%%">
        <tr><td class="%s" width="100%%">
        <table border="0" width="100%%" cellspacing="2" cellpadding="0">
        <tr><td width="100%%" class="%s">
        """ % (self.borderclass, self.titleclass)


        self.fontsize = self.fontsize + 1
        if self.fontsize == 0:
            fontsize = ''
        else:
            fontsize = ' size="%d" ' % (self.fontsize,)

        if self.more:
            two = """
            <table border="0" width="100%%" cellpadding="0" cellspacing="0">
            <tr height="28" valign="middle"><td height="28" align="center" %s>
            <font face="%s" color="%s" %s>
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>%s</b>
            </font>
            </td><td align="right" width="80" %s>
            <font face="%s" %s>
            <a href="%s">more&nbsp;...</a>
            </font>
            </td></tr>
            </table>
            """ % (titleimg, self.fontface, self.title_color, fontsize, self.title, titleimg_r, self.fontface, fontsize, self.more)
        else:
            two = """
            <table border="0" width="100%%" cellpadding="0" cellspacing="0">
            <tr height="28" valign="middle"><td height="28" align="center" %s>
            <font face="%s" color="%s" %s>
            &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<b>%s</b>
            </font>
            </td><td align="right" width="80" %s>
            <font face="%s" %s>
            &nbsp;
            </font>
            </td></tr>
            </table>
            """ % (titleimg, self.fontface, self.title_color, fontsize, self.title, titleimg_r, self.fontface, fontsize)            

        self.fontsize = self.fontsize - 1
        if self.fontsize == 0:
            fontsize = ''
        else:
            fontsize = ' size="%d" ' % (self.fontsize,)

        three = """
        </td></tr>
        <tr><td class="%s">
        <table width="100%%" border="0" cellspacing="10">
        <tr><td>
        <font face="%s" %s>
        """ % (self.contentclass, self.fontface, fontsize)


        return one + two + three

    def finish(self):
        if self.show_bottom_more:
            if self.fontsize == 0:
                fontsize = ''
            else:
                fontsize = ' size="%d" ' % (self.fontsize,)
                
            one = """
            <table border="0" width="100%%"><tr><td align="right"><font face="%s" %s><a href="%s">more ...</a></font></td></tr></table>
            """ % (self.fontface, fontsize, self.more)
        else:
            one = ''

        two = """
        </font>
        </td></tr>
        </table>
        </td></tr>
        </table>
        </td></tr>
        </table>
        """

        return one + two
    





