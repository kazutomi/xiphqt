# softwarenews.py
#
# This class manipulates rows in the softwarenews table.
#

import string
from globalfuncs import *

class SoftwareNews:
    def __init__(self, id=None, software=None, title=None, blurb=None, date_added=None):
        if id:
            self.id = int(id)
        else:
            self.id = None

        if software:
            self.software = int(software)
        else:
            self.software = None
            
        self.title = title
        self.blurb = blurb
        self.date_added = date_added

    def html_blurb(self):
        if not self.blurb: return ''
        
        return t2h(self.blurb)

    def load(self):
        from siteconfig import database
        c = database.cursor()

        c.execute('SELECT software_id, title, blurb, date_added FROM software_news WHERE software_news_id = %d' % (self.id,))
        d = c.dictfetchone()

        self.software = d['software_id']
        self.title = d['title']
        self.blurb = d['blurb']
        self.date_added = d['date_added']

    def save(self):
        if not self.software or not self.title or not self.blurb:
            return 0
        
        from siteconfig import database

        c = database.cursor()

        if self.id >= 0:
            query = "UPDATE software_news SET software_id = %d, title = '%s', blurb = '%s' WHERE software_news_id = %d" % (self.software, self.title, self.blurb, self.id)
        else:
            query = "INSERT INTO software_news (software_id, title, blurb, date_added) VALUES (%d, '%s', '%s', CURRENT_TIMESTAMP)" % (self.software, self.title, self.blurb)

        c.execute(query)

        return 1

    def delete(self):
        if not self.id: return

        from siteconfig import database
        c = database.cursor()

        c.execute("DELETE FROM software_news WHERE software_news_id = %d" % (self.id,))

class SoftwareNewsList:
    def __init__(self, num=None):
        from siteconfig import database

        self.list = []
        query = "SELECT software_news_id, software_id, title, blurb, date_added FROM software_news ORDER BY date_added DESC"
        if num > 0:
            query = query + " LIMIT %d" % (num,)

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(SoftwareNews(id=r['software_news_id'], software=r['software_id'], title=r['title'], blurb=r['blurb'], date_added=r['date_added']))
            

def get_softwarenews_box(num=None):
    from siteconfig import database

    c = database.cursor()

    # grab the most recent num music sites
    if num:
        c.execute("SELECT software_id, title, blurb, date_added FROM software_news ORDER BY date_added DESC LIMIT %d" % (num,))
    else:
        c.execute("SELECT software_id, title, blurb, date_added FROM software_news ORDER BY date_added DESC")
        
    results = c.dictfetchall()

    if not results:
        return ""

    from infobox import InfoBox
    box = InfoBox()

    box.title = "New/Updated Software"
    box.more = "/softwarenews.psp"
    box.contentclass = 'bluecontent'
    box.titleclass = 'bluetitle'
    box.borderclass = 'blueborder'
    box.fontsize = -2
    box.title_color = "#efefef"
    box.title_image = '/images/InfoBox-bg.png'
    box.title_image_r = '/images/InfoBox-right-bg.png'

    one = box.start()

    two = []
    for r in results:
        two.append("""
        <a href="/software.psp?id=%s"><font size="-1"><b>%s</b></font></a><br>
        added on <i>%s</i><br>
        %s<br>
        <br>
        """ % (r['software_id'], r['title'], r['date_added'], r['blurb']))
        
    three = box.finish()

    return one + string.join(two, '') + three
      


