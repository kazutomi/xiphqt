import ports
import shlex
from os import path
import ConfigParser

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

def parse_boolean(s):
    s = s.lower()

    if s == "yes" or s == "y" or s == "true":
        return True
    elif s == "no" or s == "n" or s == "false":
        return False
    else:
        raise Error()

# Generate the standard list of word characters
s = shlex.shlex()
wordchars = s.wordchars + "/.:~"

def quote_string(s):
    safe = True
    for ch in s:
        if ch not in wordchars:
            safe = False
            break

    if not safe:
        if '"' in s:
            return "'"+s+"'"
        else:
            return '"'+s+'"'
    else:
        return s

def strip_quotes(s):
    if len(s) >= 2:
        if    (s[0] == '"' and s[-1] == '"') \
           or (s[0] == "'" and s[-1] == "'"):
            return s[1:-1]
        else:
            return s
    else:
        return s


class Config:
    def __init__(self):
        self.config_filename = ports.user_config_file_path()
        self.mountpoint = None
        self.recording_dir = None
        self.neuros_musicdir = "MUSIC"
        self.sort_database = True
        self.syncdirs = []



    def _read_key_value_pair(self, tokenizer):
        key = tokenizer.get_token()
        equals = tokenizer.get_token()
        value = tokenizer.get_token()

        if key != "" and equals == "=" and value != "":
            return (key, strip_quotes(value))
        else:
            raise Error(tokenizer.error_leader()
                        +"Incorrect option syntax: %s %s %s"
                        % (key, equals, value))
        
    def _read_sync_section(self, tokenizer):
        startline = tokenizer.lineno
        token1 = tokenizer.get_token()
        token2 = tokenizer.get_token()

        if token1 != "begin":
            raise Error(tokenizer.error_leader()
                        +"Sync block did not start with 'begin'")
        if token2 != "sync":
            raise Error(tokenizer.error_leader()
                        +"Sync block did not start with 'begin sync'")

        token = tokenizer.get_token()
        src = None
        dest = None
        while token != "":
            if token == "end":
                token2 = tokenizer.get_token()
                if token2 == "sync":
                    if src == None:
                        raise Error(tokenizer.error_leader()
                                    +"sync block")
                    else:
                        self.syncdirs.append([src, dest])
                        break
                else:
                    raise Error(tokenizer.error_leader()
                                +"Must close sync block with 'end sync'")
            else:
                # Assume key/value pair
                tokenizer.push_token(token)
                (key, value) = self._read_key_value_pair(tokenizer)

                if key == "src":
                    src = path.expanduser(value)
                elif key == "dest":
                    dest = path.expanduser(value)
                else:
                    print tokenizer.error_leader() \
                          + "Ignoring unknown option %s" % (key,)

            token = tokenizer.get_token()
        else:
            raise Error(tokenizer.error_leader(line=startline)
                        +"sync block is never closed")


    def read_config_file(self, filename=None):
        if (filename == None):
            filename = self.config_filename
            
        f = file(filename, "r")

        tokenizer = shlex.shlex(f)
        tokenizer.wordchars = wordchars 

        token = tokenizer.get_token()
        while token != "":
            if token == "begin":
                token2 = tokenizer.get_token()
                if token2 == "sync":
                    tokenizer.push_token(token2)
                    tokenizer.push_token(token)
                    self._read_sync_section(tokenizer)
                else:
                    raise Error(tokenizer.error_leader()
                                +"Unknown block '%s'" % (token2,))
            else:
                # Assume key/value pair
                tokenizer.push_token(token)
                (key, value) = self._read_key_value_pair(tokenizer)

                if key == "mountpoint":
                    self.mountpoint = path.expanduser(value)
                elif key == "recordingdir":
                    self.recordingdir = path.expanduser(value)
                elif key == "neuros_musicdir":
                    self.neuros_musicdir = value
                elif key == "sort_database":
                    try:
                        self.sort_database = parse_boolean(value)
                    except Error:
                        raise Error(tokenizer.error_leader()
                                    +"Non boolean value '%s' given for %s",
                                    (value, key))
                else:
                    print tokenizer.error_leader() \
                          + "Ignoring unknown option %s" % (key,)

            token = tokenizer.get_token()

    def write_config_file(self, filename):
        f = file(filename, "w")

        if self.mountpoint != None:
            f.write("mountpoint=%s\n" % (quote_string(self.mountpoint),))
        if self.recordingdir != None:
            f.write("recordingdir=%s\n" % (quote_string(self.recordingdir),))
        if self.neuros_musicdir != None:
            f.write("neuros_musicdir=%s\n"
                    % (quote_string(self.neuros_musicdir),))
        if self.sort_database:
            sort_database_value = "true"
        else:
            sort_database_value = "false"
            
        f.write("sort_database=%s\n" % (sort_database_value,))

        for (src,dest) in self.syncdirs:
            f.write("\nbegin sync\n")
            f.write("  src=%s\n" % (quote_string(src),))
            if dest != None:
                f.write("  dest=%s\n" % (quote_string(dest),))
            f.write("end sync\n")
        
        f.close()
        
