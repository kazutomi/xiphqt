# musicsite.py
#
# This class manipulates rows in the musicsite table.
#

import string
from globalfuncs import *

class MusicSite:
    def __init__(self, id=None, url=None, name=None, blurb=None, date_added=None):
        if id:
            self.id = int(id)
        else:
            self.id = None
        self.url = url
        self.name = name
        self.blurb = blurb
        self.date_added = date_added


    def html_blurb(self):
        return t2h(self.blurb)

    def load(self):
        if not self.id: return 0

        from siteconfig import database
        c = database.cursor()

        c.execute('SELECT musicsite_id, url, name, blurb, date_added FROM musicsite WHERE musicsite_id = %d' % (self.id,))
        d = c.dictfetchone()

        self.id = d['musicsite_id']
        self.url = d['url']
        self.name = d['name']
        self.blurb = d['blurb']
        self.date_added = d['date_added']

    def save(self):
        if not self.url or not self.name or not self.blurb:
            return 0

        from siteconfig import database
        c = database.cursor()

        if self.id >= 0:
            query = "UPDATE musicsite SET url = '%s', name = '%s', blurb = '%s' WHERE musicsite_id = %d" % (self.url, dbesc(self.name), dbesc(self.blurb), self.id)
        else:
            query = "INSERT INTO musicsite (url, name, blurb, date_added) VALUES ('%s', '%s', '%s', CURRENT_TIMESTAMP)" % (self.url, dbesc(self.name), dbesc(self.blurb))

        c.execute(query)

        return 1

    def delete(self):
        if not self.id: return

        from siteconfig import database
        c = database.cursor()

        c.execute("DELETE FROM musicsite WHERE musicsite_id = %d" % (self.id,))


class MusicSiteList:
    def __init__(self, num=None):
        from siteconfig import database
        
        self.list = []
        query = "SELECT musicsite_id, url, name, blurb, date_added FROM musicsite ORDER BY date_added DESC"
        if num > 0:
            query = query + " LIMIT %d" % (num,)

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(MusicSite(id=r['musicsite_id'], url=r['url'], name=r['name'], blurb=r['blurb'], date_added=r['date_added']))        
        
    
def get_musicsite_box(num=None):
    msl = MusicSiteList(num)
    if not msl.list:
        return ""

    from infobox import InfoBox
    box = InfoBox()

    box.titleclass = "bluetitle"
    box.borderclass = "blueborder"
    box.contentclass = "bluecontent"
    box.title = "New Music Sites"
    box.title_color = "#efefef"
    box.more = "/musicsites.psp"
    box.fontsize = -2
    box.title_image = '/images/InfoBox-bg.png'
    box.title_image_r = '/images/InfoBox-right-bg.png'

    one = box.start()

    two = []
    for ms in msl.list:
        two.append("""
        <a href="%s"><font size="-1"><b>%s</b></font></a><br>
        <font size="-2">added on <i>%s</i></font><br>
        <font size="-1">%s</font><br>
        <br>
        """ % (ms.url, ms.name, ms.date_added, ms.html_blurb()))
    three = box.finish()

    return one + string.join(two, '') + three
      


