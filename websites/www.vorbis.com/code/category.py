# category.py
#
# This class manipulates rows in the category table
#

import string
from globalfuncs import *

class Category:
    def __init__(self, id=None, name=None, blurb=None):
        if id:
            self.id = int(id)
        else:
            self.id = None
            
        self.name = name
        self.blurb = blurb

    def html_blurb(self):
        return t2h(self.blurb)
        
    def load(self):
        if not self.id: return
        
        from siteconfig import database
        
        c = database.cursor()
        
        c.execute('SELECT name, blurb FROM category WHERE category_id = %d' % (self.id,))
        d = c.dictfetchone()
            
        self.name = d['name']
        self.blurb = d['blurb']

    def save(self):
        if not self.name or not self.blurb:
            return 0

        from siteconfig import database
        c = database.cursor()

        if self.id >= 0:
            query = "UPDATE category SET name = '%s', blurb = '%s' WHERE category_id = %d" % (dbesc(self.name), dbesc(self.blurb), self.id)
        else:
            query = "INSERT INTO category (name, blurb) VALUES ('%s', '%s')" % (dbesc(self.name), dbesc(self.blurb))

        c.execute(query)

        return 1

    def delete(self):
        if not self.id: return

        from siteconfig import database
        c = database.cursor()

        c.execute("DELETE FROM category WHERE category_id = %d" % (self.id,))
        
class CategoryList:
    def __init__(self):
        from siteconfig import database
        
        self.list = []
        query = "SELECT category_id, name, blurb FROM category ORDER BY name"

        c = database.cursor()
        c.execute(query)

        results = c.dictfetchall()
        for r in results:
            self.list.append(Category(id=r['category_id'], name=r['name'], blurb=r['blurb']))
            
    


