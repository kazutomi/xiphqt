#!/usr/bin/env python
#
#       Line.py
#       
#       Copyright 2008 Joao Mesquita <jmesquita@gmail.com>
#       
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the GNU General Public License as published by
#       the Free Software Foundation; either version 3 of the License, or
#       (at your option) any later version.
#       
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       GNU General Public License for more details.
#       
#       You should have received a copy of the GNU General Public License
#       along with this program; if not, write to the Free Software
#       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#       MA 02110-1301, USA.


class Line:
    """
        Each line of a subtile will have its own class to control
        the number of characters and other features
    """
    def __init__(self, text):
        """
            Each line has its own text
        """
        self.text = text
        self.length = self._count(text)

    def _count(self, text):
        """
            We have to make our own count
            function because of pango markups
            and end of lines.
        """
        t = text.strip('\n')
        return len(t)
