# -*- Mode: python -*-
#
# progress.py - progress meter
#
# Copyright (C) 2003, Xiph.org Foundation
#
# This file is part of positron.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of a BSD-style license (see the COPYING file in the
# distribution).
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the license for more details.

from sys import stderr

class Progress:
    
    def sync_start(self, num_items):
        stderr.write("Downloading %d files...\n" % (num_items,))
        return true

    def transfer_start(self, filename, size):
        stderr.write("%s:\n")

    def transfer_progress(self, filename, size, bytes_so_far, rate):
        self._print_line(filename, size, bytes_so_far, rate)

    def transfer_stop(self, filename, size):
        self._print_line(filename, size, size, 0)
        stderr.write("\n")

    def sync_stop(self):
        pass

    def _print_line(self, filename, size, bytes_so_far, rate):
        percent_str = "%3d%%" % (100 * bytes_so_far / size,)

        max_bar_width = 38
        if bytes_so_far < size:
            bar_width = max_bar_width * bytes_so_far / size
            bar = "=" * (bar_width - 1) + ">"
        else:
            bar = "=" * max_bar_width

        size_str = self._gen_size_str(size)

        if rate > 0:
            rate_str = self._gen_size_str(rate) + "/s"

            if bytes_so_far < size:
                time_remaining = (size - bytes_so_far) / rate
                seconds = time_remaining % 60
                minutes = (time_remaining // 60) % 60
                hours = time_remaining // 3600

                if hours > 0:
                    eta_str = "ETA: %d:%02d:%02d" % (hours, minutes, seconds)
                else:
                    eta_str = "ETA: %d:%02d" % (minutes, seconds)
            else:
                eta_str = ""
        else:
            rate_str = ""
            eta_str = ""
            
        str = "%-4s|%-*s|%8s  %10s  %-15s" % (percent_str, max_bar_width,
                                           bar, size_str, rate_str, eta_str)
        str = "\r" + " " * 78 + "\r" + str
        stderr.write(str)

    def _gen_size_str(self, size):
        kB = 1024.0
        MB = kB * 1024.0
        GB = MB * 1024.0
        if size < kB:
            size_str = "%dB" % (size, )
        elif size < MB:
            size_str = "%1.1fkB" % (size/kB,)
        elif size < GB:
            size_str = "%1.1fMB" % (size/MB,)
        else:
            size_str = "%1.1fGB" % (size/GB,)
        return size_str
                            
