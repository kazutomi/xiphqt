# software.py
#
# This class manipulates rows in the software table
#

import string
from globalfuncs import *

class Software:
    def __init__(self, id=None, platform=[], url=None, name=None, blurb=None, date_added=None):
        if id:
            self.id = int(id)
        else:
            self.id = None

        self.platform = []
        if platform:
            for p in platform:
                self.platform.append(int(p))

            
        self.url = url
        self.name = name
        self.blurb = blurb
        self.date_added = date_added

    def html_blurb(self):
        if not self.blurb: return ''

        return t2h(self.blurb)

    def load(self):
        if not self.id: return
        
        from siteconfig import database
        c = database.cursor()

        c.execute('SELECT url, name, blurb, date_added FROM software WHERE software_id = %d' % (self.id,))
        d = c.dictfetchone()

        self.url = d['url']
        self.name = d['name']
        self.blurb = d['blurb']
        self.date_added = d['date_added']

        c.execute("SELECT platform_id FROM software_platform WHERE software_id = %d" % (self.id,))
        results = c.dictfetchall()

        for r in results:
            self.platform.append(r['platform_id'])


    def save(self):
        if not self.url or not self.name or not self.blurb:
            return 0
        
        from siteconfig import database
	database.autocommit(0)
        c = database.cursor()
	
        if self.id >= 0:
             c.execute("UPDATE software SET url = '%s', name = '%s', blurb = '%s' WHERE software_id = %d" % (self.url, dbesc(self.name), dbesc(self.blurb), self.id))
             c.execute("DELETE FROM software_platform WHERE software_id = %d" % (self.id,))
             for p in self.platform:
                 c.execute("INSERT INTO software_platform (software_id, platform_id) VALUES (%d, %d)" % (self.id, p))
                 
             database.commit()
        else:
            c.execute("SELECT nextval('software_software_id_seq') as pk")
            d = c.dictfetchone()
            id = d['pk']
            
            c.execute("INSERT INTO software (software_id, url, name, blurb, date_added) VALUES (%d, '%s', '%s', '%s', CURRENT_TIMESTAMP)" % (id, self.url, dbesc(self.name), dbesc(self.blurb)))

            for p in self.platform:
                c.execute("INSERT INTO software_platform (software_id, platform_id) VALUES (%d, %d)" % (id, p))

            database.commit()

	database.autocommit(1)
        return 1

    def delete(self):
        if not self.id: return

        from siteconfig import database
	database.autocommit(0)
        c = database.cursor()

        c.execute("DELETE FROM software_platform WHERE software_id = %d" % (self.id,))
        c.execute("DELETE FROM software_news WHERE software_id = %d" % (self.id,))
        c.execute("DELETE FROM software WHERE software_id = %d" % (self.id,))
        database.commit()
	database.autocommit(1)

class SoftwareList:
    def __init__(self, num=None, platform=None):
        from siteconfig import database
        
        self.list = []
        
        if not platform:
            query = "SELECT software_id, url, name, blurb, date_added FROM software ORDER BY name"
        else:
            query = "SELECT software_id, url, name, blurb, date_added FROM software WHERE software_id in (SELECT software_id FROM software_platform WHERE platform_id = %d) ORDER BY name" % (platform,)
            
        if num > 0:
            query = query + " LIMIT %d" % (num,)

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(Software(id=r['software_id'], url=r['url'], name=r['name'], blurb=r['blurb'], date_added=r['date_added']))
    



