# news.py
#
# This class manipulates rows in the news table
#

import string
from globalfuncs import *

class News:
    def __init__(self, id=None, type=None, poster=None, title=None, blurb=None, date_added=None, iconfile=None):
        if id:
            self.id = int(id)
        else:
            self.id = None
        if type:
            self.type = int(type)
        else:
            self.type = None
        self.poster = poster
        self.title = title
        self.blurb = blurb
        self.date_added = date_added
        self.iconfile = iconfile

    def html_blurb(self):
        if not self.blurb: return ''

        return t2h(self.blurb)

    def load(self):
        if not self.id: return
        
        from siteconfig import database

        c = database.cursor()

        c.execute('SELECT iconfile, type.type_id as tid, poster, title, blurb, date_added FROM news, type WHERE news.type_id = type.type_id AND news_id = %d' % (self.id,))
        d = c.dictfetchone()

        self.type = d['tid']
        self.poster = d['poster']
        self.title = d['title']
        self.blurb = d['blurb']
        self.date_added = d['date_added']
        self.iconfile = d['iconfile']


    def save(self):
        if not self.type or not self.poster or not self.title or not self.blurb:
            return 0
        
        from siteconfig import database
        
        c = database.cursor()

        if self.id >= 0:
            query = "UPDATE news SET type_id = %d, poster = '%s', title = '%s', blurb = '%s' WHERE news_id = %d" % (self.type, dbesc(self.poster), dbesc(self.title), dbesc(self.blurb), self.id)
        else:
            query = "INSERT INTO news (type_id, poster, title, blurb, date_added) VALUES (%d, '%s', '%s', '%s', CURRENT_TIMESTAMP)" % (self.type, dbesc(self.poster), dbesc(self.title), dbesc(self.blurb))

        c.execute(query)

        return 1

    def delete(self):
        if not self.id: return

        from siteconfig import database
        c = database.cursor()

        c.execute("DELETE FROM news WHERE news_id = %d" % (self.id,))

class NewsList:
    def __init__(self, num=None):
        from siteconfig import database
        
        self.list = []
        query = "SELECT news_id, type.type_id as tid, iconfile, poster, title, blurb, date_added FROM news, type WHERE news.type_id = type.type_id ORDER BY date_added DESC"
        if num > 0:
            query = query + " LIMIT %d" % (num,)

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(News(id=r['news_id'], type=r['tid'], iconfile=r['iconfile'], poster=r['poster'], title=r['title'], blurb=r['blurb'], date_added=r['date_added']))
    

def get_news_box(num=None):
    nl = NewsList(num)
    
    if not nl.list:
        return ""

    from infobox import InfoBox
    box = InfoBox()

    box.title = "Ogg Vorbis News"
    box.more = "/news.psp"
    box.fontsize = -1
    box.title_image = '/images/NewsBox-bg.png'
    box.title_image_r = '/images/NewsBox-right-bg.png'
    box.show_bottom_more = 1
    
    one = box.start()

    two = []
    for n in nl.list:
        two.append("""
        <img src="%s" align="left" width="32" height="32" alt="icon">
        <b><font size="+1">%s</font></b><br>
        <font size="-2">posted by <b>%s</b> on <i>%s</i></font><br>
        <br>
        %s<br>
        <br>
        <br>
        """ % (n.iconfile, n.title, n.poster, n.date_added, n.html_blurb()))
    three = box.finish()

    return one + string.join(two, '') + three
      



