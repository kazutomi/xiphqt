#!/usr/bin/env python
#
#       Sub.py
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


class Sub:
    """
        The Sub class, is the class that handles each subtitle 
        individually
    """

    def __init__(self):
        """
            Lets init all the variables
            This might work more or less like a C struct
        """
        self.text=""
        self.start_time=None
        self.end_time=None
        self.Attributes=None
        self.number=None

    ## Check subtitle time.
    # This function check if subtitle visibility in given time.
    # \param[in] time - time to check
    # \return 1 - if visibility in time, 0 - otherwise
    def isInTime(self, time):
        if( (time>=self.start_time) and (time<=self.end_time) ):
            return 1
        else:
            return 0

    ## \var text
    # A variable to store subtitle text
    
    ## \var start_time
    # A variable to store a start time of visibility of subtitle (in ns).
    
    ## \var end_time
    # A variable to store a end time of visibility of subtitle (in ns).
    
    ## \var Attributes
    # A array of attributes of subtitle. (NOT USED YET)

#==============================================================================
