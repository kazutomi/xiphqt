import ports
from os import path
import ConfigParser

class Config(dict):
    def __init__(self):
        self.config_filename = ports.user_config_file_path()
        self.config_file = ConfigParser.ConfigParser()
        self.mountpoint = None
        self.musicdir = "MUSIC"

    def check_config_file(self):
        return path.isfile(self.config_filename)

    def read_config_file(self):
        self.config_file.read(self.config_filename)

        if self.mountpoint != None \
               and config.has_option("general","mountpoint"):
            self.mountpoint = config.get("general","mountpoint")

        if config.has_option("general","musicdir"):
            self.musicdir = config.get("general","musicdir")
        
    def write_config_file(self):
        f = file(self.config_filename, "w")
        self.config_file.write(f)
        f.close()
        
