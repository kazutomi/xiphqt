# type.py
#
# This class manipulates rows in the type table
#

import string

class Type:
    def __init__(self, id=None, name=None, iconfile=None):
        self.id = id
        self.name = name
        self.iconfile = iconfile
        
    def load(self):
        if not self.id: return
        
        from siteconfig import database
        
        c = database.cursor()
        
        c.execute('SELECT name, iconfile FROM type WHERE type_id = %d' % (self.id,))
        d = c.dictfetchone()
            
        self.name = d['name']
        self.iconfile = d['iconfile']        


class TypeList:
    def __init__(self):
        from siteconfig import database
        
        self.list = []
        query = "SELECT type_id, name, iconfile FROM type ORDER BY name"

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(Type(id=r['type_id'], name=r['name'], iconfile=r['iconfile']))
            
    


