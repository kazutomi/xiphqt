# platform.py
#
# This class manipulates rows in the platform table
#

import string

class Platform:
    def __init__(self, id=None, name=None):
        if id:
            self.id = id
        else:
            self.id = None
        self.name = name
        
    def load(self):
        if not self.id: return
        
        from siteconfig import database
        c = database.cursor()
        
        c.execute('SELECT name FROM platform WHERE platform_id = %d' % (self.id,))
        d = c.dictfetchone()
            
        self.name = d['name']

class PlatformList:
    def __init__(self):
        from siteconfig import database
        
        self.list = []
        query = "SELECT platform_id, name FROM platform ORDER BY name"

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(Platform(id=r['platform_id'], name=r['name']))
            
    


