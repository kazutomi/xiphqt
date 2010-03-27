/* Castor, a fast Vorbis decoder created by Michael Scheerer.
 *
 * Castor decoder (c) 2010 Michael Scheerer www.meviatronic.com
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


package com.meviatronic.zeus.castor;

import org.xiph.ogg.*;

import java.io.*;

/**
 * The <code>Output</code> class implements all output related methods of the decoder.
 * Furthermore all output channel related settings are performed
 *
 * @author    Michael Scheerer
 */
abstract public class Output {

  	AudioReader info;
	int channels;
	byte[] buffer; // inlining
	int[] pointer; // inlining
	
	private int frameBufferSize;
	private int frameBufferSizePerChannel;
	private long granulepos;
	private long lastGranulepos;
	private int sampleOffset;
	private boolean eom;
	private boolean bom;
  	
	Output(AudioReader info){
		
		this.info = info;
		
		channels = info.channels;
		
		frameBufferSizePerChannel = info.blocksizes[1];
		int frameBufferSize = frameBufferSizePerChannel * channels;

		buffer = new byte[frameBufferSize];
		pointer = new int[channels];
	}

	abstract void decode() throws IOException, EndOfMediaException, EndOfPacketException;
	
	public final byte[] synthesis(Packet op) throws IOException, EndOfPacketException {
    	info.loadPacket(op.packetBase, op.packet, op.bytes);
		
		eom = op.eos == 1 ? true : false;
		bom = op.bos == 1 ? true : false;

		resetBufferPointer();
		
		decode();
		
		if (op.granulepos == -1) {
			granulepos = lastGranulepos + frameBufferSizePerChannel;
		} else {
			granulepos = op.granulepos;
		}
		sampleOffset = 0;
		if (granulepos < 0 && bom) {
			frameBufferSizePerChannel += granulepos;
			sampleOffset = (int) -granulepos;
			if (frameBufferSizePerChannel < 0) {
				frameBufferSizePerChannel = 0;
			}
		}
		if (granulepos - lastGranulepos < frameBufferSizePerChannel && eom) {
			frameBufferSizePerChannel = (int) (granulepos - lastGranulepos);
		}
		lastGranulepos = granulepos;
		return buffer;
	}
	
  	public final int getNumberOfSamples() {
		return frameBufferSizePerChannel;
  	}
	
	public final int getSampleOffset() {
		return sampleOffset;
	}

  	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		int i;
		
		buffer = null;
		pointer = null;
	}
	
	final void setBlockSize(int bufferSize, int spectrumSize) {
		frameBufferSizePerChannel = bufferSize << 1;
		frameBufferSize = frameBufferSizePerChannel * channels;
	}
	
	final void setBuffer(float f, int channelNumber) {
		
		if (f >= 1) {
			f = 1;
		}
		if (f <= -1) {
			f = -1;
		}
	
		f *= 32767;

		short w = (short) f;
		// one short with big-endian order to 2 bytes in big-endian order!!
		byte b1 = (byte) (w >>> 8);
		byte b2 = (byte) w;
		
		int p = pointer[channelNumber];
		
		buffer[p] = b1;
		buffer[p + 1] = b2;
			
		p += channels << 1;
		pointer[channelNumber] = p;
	}
	
	private void resetBufferPointer() {
		for (int i = 0; i < channels; i++) {
			pointer[i] = 2 * i;
		}
	}
	
	final boolean bookIsUnused(int bookNumber) {
		return info.bookIsUnused(bookNumber);
	}
	
	final int get1() throws EndOfPacketException {
		return info.getLittleEndian1();
	}
	
	final int get(int i) throws EndOfPacketException {
		return info.getLittleEndian(i);
	}
	 
	final int getCodeWord(int bookNumber) throws IOException, EndOfPacketException {
		return info.getCodeWord(bookNumber);
	}
	
	final static int ilog(int v) {
		return AudioReader.ilog(v);
	}
}
