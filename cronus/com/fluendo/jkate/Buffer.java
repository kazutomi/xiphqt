/* Castor, a fast Vorbis decoder created by Michael Scheerer, adapted and
 * integrated into a stripped fork of the jorbis framework.
 *
 * Castor decoder (c) 2009 Michael Scheerer www.meviatronic.com
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

package com.fluendo.jkate;

import java.io.*;

/**
 * Bitstream processor for little endian style bitstream processing.
 * Rewritten from:
 *
 * @author    Michael Scheerer
 */
public class Buffer{
  	private static final int BUFFER_INCREMENT=256;
	
	protected final static int BYTELENGTH = 8;
	protected final static int DOUBLE_BYTELENGTH = 16;
	protected final static int TRIPLE_BYTELENGTH = 24;
	protected final static int QUADRUPEL_BYTELENGTH = 32;

	protected final static int BITMASK[] = {
		0x00000000,
		0x00000001, 0x00000003, 0x00000007, 0x0000000F,
		0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
		0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
		0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
		0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
		0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
		0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
		0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
	};

	private int byteIdx;
	private byte[] data;
	private int revBitIdx;
	private int packetByteIdx;
	private int packetSize;
	
  	public void read(byte[] s, int bytes) {
    	int i=0;
    	while(bytes--!=0){
      		s[i++]=(byte)(read(8));
    	}
  	}
	
  	public void loadPacket(byte[] buf, int bytes) {
    	loadPacket(buf, 0, bytes);
	}
	
	public void loadPacket(byte[] buf, int start, int bytes){
    	byteIdx = start;
    	data = buf;
    	revBitIdx = packetByteIdx = 0;
    	packetSize = bytes;
	}
	
	public final int look1() {
		
	  	if (handleBounderies(1) < 0) {
		  	return -1;
		}

		int val = data[byteIdx] >>> revBitIdx & 1;

		return val;
	}

	public final int read1() {
		
	  	if (handleBounderies(1) < 0) {
		  	return -1;
		}

		int val = data[byteIdx] >>> revBitIdx & 1;

		revBitIdx++;
		if (revBitIdx == BYTELENGTH) {
			revBitIdx = 0;
			byteIdx++;
			packetByteIdx++;
			
		}

		return val;
	}
	
	public final int read(int i) {
		if (i <= 0) {
			return 0;
		}
		if (handleBounderies(i + revBitIdx) < 0) {
			return -1;
		}

		int store = revBitIdx;
		
		int val = (data[byteIdx] & 0xFF) >>> store;
		
		revBitIdx += i;
		
		if (revBitIdx >= BYTELENGTH) {
			byteIdx++;
			packetByteIdx++;
			val |= (data[byteIdx] & 0xFF) << BYTELENGTH - store;
			if (revBitIdx >= DOUBLE_BYTELENGTH) {
				byteIdx++;
				packetByteIdx++;
				val |= (data[byteIdx] & 0xFF) << DOUBLE_BYTELENGTH - store;
				if (revBitIdx >= TRIPLE_BYTELENGTH) {
					byteIdx++;
					packetByteIdx++;
					val |= (data[byteIdx] & 0xFF) << TRIPLE_BYTELENGTH - store;
					if (revBitIdx >= QUADRUPEL_BYTELENGTH) {
						byteIdx++;
						packetByteIdx++;
						val |= ((data[byteIdx] & 0xFF) << QUADRUPEL_BYTELENGTH - store) & 0xFF000000;
					}
				}
			}
			revBitIdx &= 7;
		}
		return val & BITMASK[i];
	}
	
	final int skip(int i) {
		if (handleBounderies(i + revBitIdx) < 0) {
			return -1;
		}
		revBitIdx += i;
		while (revBitIdx >= BYTELENGTH) {
			revBitIdx -= BYTELENGTH;
			byteIdx++;
			packetByteIdx++;
		}
		return 0;
	}
	
	public int bits(){
    	return byteIdx * 8 + revBitIdx;
  	}

  	private int handleBounderies(int bits)  {
		int ret = 0;
		if(packetByteIdx + bits / 8 >= packetSize){
        	byteIdx += bits / 8;
        	packetByteIdx += bits / 8;
			revBitIdx = bits & 7;
        	return -1;
		}
		return ret;
  	}
}


