from util import *
from os import path
from MDB import MDB
from SAI import SAI
from PAI import PAI
from XIM import XIM

class WOID:

    def __init__(self):
        self.root = None
        self.name = None
        self.mdb = None
        self.sai = None
        self.pai = None
        self.children = ()
        self.extra_format = ()
        self.no_flatten = ()

    def open(self, root, extra_format, no_flatten):
        # Construct pathnames
        mdbpath = root + ".mdb"
        saipath = root + ".sai"
        paipath = root + ".pai"

        # Open DBs we know have to exist
        mdb = MDB()
        mdb.open(mdbpath)
        mdb.set_extra_format(extra_format)
        sai = SAI()
        sai.open(saipath)

        # Get our name
        name = mdb.header["DB_Name"]

        # Do we have a parent?
        if mdb.header["ParentDBFileName"] != None:
            pai = PAI()
            pai.open(paipath)
        else:
            pai = None

        # Now check for child databases (which hold the values for access keys)
        children = []
        if mdb.header["NumOfKeys"] > 1:
            dirname = path.split(root)[0]
            for key in mdb.header["Rules"][1:]:
                child_name = path.splitext(key[1])[0] # Throw away extension
                child_root = path.join(dirname, child_name)

                child = WOID()
                child.open(child_root, (), ())
                children.append(child)

        # Now save all the attributes
        self.root = root
        self.name = name
        self.mdb = mdb
        self.sai = sai
        self.pai = pai
        self.children = children
        self.extra_format = extra_format
        self.no_flatten = no_flatten
        
    def _get_record_at(self, pointer):
        mdb_record = self.mdb.read_record_at(pointer)[0]

        if mdb_record != None:  # Only have to process non-empty records
            record = [mdb_record["data"]]

            # Append access key fields (looking up their values in child DBs
            for i in range(len(mdb_record["keys"])):
                bag = []
                for child_ptr in mdb_record["keys"][i]:
                    child_record = self.children[i]._get_record_at(child_ptr)
                    if child_record == None:
                        bag.append(None)
                    else:
                        bag.append(child_record[0])

                record.append(bag)

            # Append extra data fields
            record.extend(mdb_record["extra"])

            # Flatten records if necessary
            for i in range(len(record)):
                if i not in self.no_flatten:
                    record[i] = flatten_singlet(record[i])
                else:
                    record[i] = collapse_null_list(record[i])
        else:
            record = None


        return record

    def get_records(self):
        """Returns a list of all the records in this database"""

        return [self._get_record_at(pointer[0]) for pointer in self.sai]

    def find(self, data):

        for sai_record in self.sai:
            record = self.mdb.read_record_at(sai_record[0])[0]
            if record == None:
                if data == None:
                    break
            elif data in record["data"]:
                break
        else:
               sai_record = None   # Could not find record

        return sai_record

    def add_record(self, record):
        # Call internal add record function but discard return values
        self._add_record(record)

    def _add_record(self, record):
        """Adds a record to this db and returns a SAI tuple for it.
        First element of return value is a word pointer to the MDB
        record and the second element is a word pointer to the PAI
        module corresponding to new record."""

        # Put single objects into lists
        record = map(unflatten_singlet, record)

        # Build record hash
        mdb_record = {"isDeleted" : False,
                      "data" : record[0],
                      "keys" : [],
                      "extra" : record[self.mdb.header["NumOfKeys"]:]}

        # Need to lookup keys
        child_pai_modules = []
        for i in range(1,self.mdb.header["NumOfKeys"]):
            bag = []
            for key in record[i]:
                
                # Find the pointers to child records corresponding to
                # each of the access keys and queue up pointers to PAI
                # modules for each child record so we can update them
                # later once we know the pointer to the main record we
                # are adding
                sai_record = self.children[i-1].find(key)
            
                if sai_record == None:
                    # Need to add this key to child database
                    child_record = (key,)
                    sai_record = self.children[i-1]._add_record(child_record)

                # MDB pointer to matching record in child db
                pointer = sai_record[0]
                if (sai_record[1] != 0):  # Don't update if PAI module pointer is zero
                    child_pai_modules.append((self.children[i-1].pai, sai_record[1]))

                bag.append(pointer)
                
            mdb_record["keys"].append(bag)

        # Now update all the databases
        mdb_pointer = self.mdb.append_record(mdb_record)

        if self.pai != None:
            pai_pointer = self.pai.append_module(())
        else:
            pai_pointer = 0

        new_sai_record = (mdb_pointer, pai_pointer) 
        self.sai.append(new_sai_record)

        # Now we update the child PAI files with back pointers
        for child_pai, child_pai_module_ptr in child_pai_modules:
            child_pai.add_entry_to_module_at(child_pai_module_ptr, mdb_pointer)

        return new_sai_record

    def delete_record(self, filename):
        pass

    def close(self):
        self.mdb.close()
        self.sai.close()
        if self.pai != None:
            self.pai.close()
        self.__init__()

    
