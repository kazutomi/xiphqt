import os
from os import path

def site_config_file_path():
    if os.name == "nt":
        return None
    else:
        return path.join("/etc/",".positron")

def user_config_file_path():
    if os.name == "nt":
        return None
    else:
        return path.join(path.expanduser("~"),".positron")
