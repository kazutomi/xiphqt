/* Helena, a Comment decoder created by Michael Scheerer.
 *
 * Helena decoder (c) 2010 Michael Scheerer www.meviatronic.com
 *
 * Many thanks to
 *   Monty <monty@xiph.org> and
 *   The XIPHOPHORUS Company http://www.xiph.org/ .
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
   
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


package com.meviatronic.zeus.helen;

import java.io.*;

/**
 * The <code>Converter</code> class can be used to transform UTF-8 data to characters.
 *
 * @author	Michael Scheerer
 */
final class Converter {

    String convert(byte b[]) {
    	return convert(b, 0, b.length);
    }
    
    String convert(byte b[], int i, int j) {
		int charIndex = 0, k, l, m, n;
		
		char onec = 0;
		
		StringBuffer buffer = new StringBuffer();
		
		for (int byteIndex = i; byteIndex < j;) {
        	
            byte oneb = b[byteIndex++];
            
            if (oneb >= 0) {
				onec = (char) oneb;
			} else {
				k = oneb & 0xFF;
				
				if ((k >>> 5 & 0x7) == 6 && byteIndex < j) {
					l = b[byteIndex++];
					onec = (char)(k << 6 & 0x1F | l & 0x3F);
				} else if ((k >>> 4 & 0xF) == 14 && byteIndex < j - 1) {
					l = b[byteIndex++];
					m = b[byteIndex++];
					onec = (char)(k << 12 & 0xF | l << 6 & 0x3F | m & 0x3F);
				} else if (byteIndex < j - 2) {
					l = b[byteIndex++];
					m = b[byteIndex++];
					n = b[byteIndex++];
					onec = (char)(k << 18 & 0x7 | l << 12 & 0x3F | m << 6 & 0x3F | n & 0x3F);
				} else {
					onec = ' ';
				}
			}
			buffer.append(onec);
		}
		return buffer.toString();
    }
}

