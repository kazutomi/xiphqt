# link.py
#
# This class manipulates rows in the link table.
#

import string
from globalfuncs import *

class Link:
    def __init__(self, id=None, cat=None, url=None, name=None, blurb=None, date_added=None, catname=None):
        if id:
            self.id = int(id)
        else:
            self.id = None
        if cat:
            self.cat = int(cat)
        else:
            self.cat = None
        self.url = url
        self.name = name
        self.blurb = blurb
        self.date_added = date_added
        self.catname = catname


    def html_blurb(self):
        if not self.blurb: return ''
        
        return t2h(self.blurb)

    def load(self):
        if not self.id: return 0

        from siteconfig import database
        c = database.cursor()

        c.execute('SELECT category.name as catname, link_id, category.category_id as cid, url, link.name as name, link.blurb as blurb, date_added FROM link, category WHERE link.category_id = category.category_id AND link_id = %d' % (self.id,))
        d = c.dictfetchone()

        self.id = d['link_id']
        self.cat = d['cid']
        self.url = d['url']
        self.name = d['name']
        self.blurb = d['blurb']
        self.date_added = d['date_added']
        self.catname = d['catname']

    def save(self):
        if not self.cat or not self.name or not self.blurb:
            return 0

        from siteconfig import database
        c = database.cursor()

        if self.id >= 0:
            query = "UPDATE link SET category_id = %d, url = '%s', name = '%s', blurb = '%s' WHERE link_id = %d" % (self.cat, self.url, dbesc(self.name), dbesc(self.blurb), self.id)
        else:
            query = "INSERT INTO link (category_id, url, name, blurb, date_added) VALUES (%d, '%s', '%s', '%s', CURRENT_TIMESTAMP)" % (self.cat, self.url, dbesc(self.name), dbesc(self.blurb))

        c.execute(query)

        return 1

    def delete(self):
        if not self.id: return

        from siteconfig import database
        c = database.cursor()

        c.execute("DELETE FROM link WHERE link_id = %d" % (self.id,))


class LinkList:
    def __init__(self, num=None):
        from siteconfig import database
        
        self.list = []
        query = "SELECT link_id, link.category_id as cid, category.name as catname, url, link.name as name, link.blurb as blurb, date_added FROM link, category WHERE link.category_id = category.category_id ORDER BY date_added DESC"
        if num > 0:
            query = query + " LIMIT %d" % (num,)

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(Link(id=r['link_id'], cat=r['cid'], url=r['url'], name=r['name'], blurb=r['blurb'], date_added=r['date_added'], catname=r['catname']))
    
def get_link_box(num=None):
    ll = LinkList(num)
    if not ll.list:
        return ""

    from infobox import InfoBox
    box = InfoBox()

    box.titleclass = "bluetitle"
    box.borderclass = "blueborder"
    box.contentclass = "bluecontent"
    box.title = "New Links"
    box.more = "/links.psp"
    box.fontsize = -2
    box.title_color = "#efefef"
    box.title_image = '/images/InfoBox-bg.png'
    box.title_image_r = '/images/InfoBox-right-bg.png'

    one = box.start()

    two = []
    for l in ll.list:
        two.append("""
        <font size="-1"><a href="%s"><b>%s</b></a></font><br>
        <font size="-2">added to <b>%s</b> on <i>%s</i></font><br>
        <font size="-1">%s</font><br>
        <br>
        """ % (l.url, l.name, l.catname, l.date_added, l.html_blurb()))
    three = box.finish()

    return one + string.join(two, '') + three
      












