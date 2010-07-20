/* Ogg, created by Xiph,
 * ported from C++ to Java and named as Jorbis framework.
 *
 * Ogg decoder (c) 2000 Xiph foundation www.xiph.org
 * Jorbis framework (c) 2000 ymnk, JCraft,Inc. <ymnk@jcraft.com>
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

package org.xiph.ogg;

/**
 * The <code>Page</code> class provides all necessary page related members.
 * The CRC algorithm is delevoped based on public domain code by
 * Ross Williams (ross@guest.adelaide.edu.au).
 * A 32 bit CRC value (direct algorithm, initial val and final XOR = 0, generator polynomial=0x04c11db7) is used.
 * The value is computed over the entire header (with the CRC field in the header set to zero)
 * and then continued over the page.
 * The CRC field is then filled with the computed value.
 * This algorithm is taken from the Castor decoder.
 *
 * @author Xiph, JCraft (Port), Michael Scheerer (CRC)
 */


public final class Page {
	
	private static int[] crcTable;
	
	static {
		int i, j, r;
		
		crcTable = new int[256];
		
		for (i = 0; i < 256; i++) {
			r = i << 24;
			
			for (j = 0; j < 8; j++) {
				if ((r & 0x80000000) != 0) {
					r = r << 1 ^ 0x04C11DB7;
				} else {
					r <<= 1;
				}
			}
			crcTable[i] = r;
		}
	}


  	public byte[] header_base;
  	public int header;
  	public int header_len;
  	public byte[] body_base;
  	public int body;
  	public int body_len;

  	int version(){
    	return header_base[header + 4] & 0xFF;
  	}

  	int continued() {
    	return (header_base[header + 5] & 0x01);
  	}

  	public int bos() {
    	return (header_base[header + 5] & 0x02);
  	}

  	public int eos() {
    	return (header_base[header + 5] & 0x04);
  	}

  	public long granulepos() {
    	long foo=header_base[header + 13] & 0xFF;
    	foo = (foo << 8) | (header_base[header + 12] & 0xFF);
    	foo = (foo << 8) | (header_base[header + 11] & 0xFF);
    	foo = (foo << 8) | (header_base[header + 10] & 0xFF);
    	foo = (foo << 8) | (header_base[header + 9] & 0xFF);
    	foo = (foo << 8) | (header_base[header + 8] & 0xFF);
    	foo = (foo << 8) | (header_base[header + 7] & 0xFF);
    	foo = (foo << 8) | (header_base[header + 6] & 0xFF);
    	return (foo);
  	}

  	public int serialno() {
    	return (header_base[header + 14] & 0xFF) | ((header_base[header + 15] & 0xFF) << 8)
        | ((header_base[header + 16] & 0xFF) << 16)
        | ((header_base[header + 17] & 0xFF) << 24);
  	}

  	int pageno() {
    	return (header_base[header + 18] & 0xFF) | ((header_base[header + 19] & 0xFF) << 8)
        | ((header_base[header + 20] & 0xFF) << 16)
        | ((header_base[header + 21] & 0xFF) << 24);
  	}
	
	private int getChecksum(int crc, byte[] data, int i, int j) {
		int k = i + j;
		
		for (; i < k; i++) {
			crc = crc << 8 ^ crcTable[crc >>> 24 & 0xFF ^ data[i] & 0xFF];
		}
		return crc;
	}
	
	private static int getChecksum(int crc, byte data) {
		return crc << 8 ^ crcTable[crc >>> 24 & 0xFF ^ data & 0xFF];
	}
	
	void checksum(){
		int crc_reg = 0;
		
	 	crc_reg = getChecksum(crc_reg, header_base, header, header_len);
		crc_reg = getChecksum(crc_reg, body_base, body, body_len);
		header_base[header + 22] = (byte)crc_reg;
    	header_base[header + 23] = (byte)(crc_reg >>> 8);
    	header_base[header + 24] = (byte)(crc_reg >>> 16);
    	header_base[header + 25] = (byte)(crc_reg >>> 24);
	}
	
	public void close () {
		header_base = null;
		body_base = null;
	}
}
