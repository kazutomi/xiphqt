import os
from os import path

# Someday we'll stick OS specific information into this file

def user_config_dir():
        return path.join(path.expanduser("~"),".positron")
