class Config(dict):
    def __init__(self, filename):
        self.filename = filename
        f = file(filename, "r")
        

    def save(self):
        f = file(filename, "w")
        
