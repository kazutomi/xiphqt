import os
from os import path


def config_file_path():
    if os.name == "nt":
        return path.join(path.expanduser("~"),"Application Data","Positron",
                         "config.txt")
    else:
        return path.join(path.expanduser("~"),".positron"

