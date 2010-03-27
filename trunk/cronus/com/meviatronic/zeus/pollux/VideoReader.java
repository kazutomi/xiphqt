/* Pollux, a fast Theora decoder created by Michael Scheerer.
 *
 * Pollux decoder (c) 2010 Michael Scheerer www.meviatronic.com
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


package com.meviatronic.zeus.pollux;

import com.meviatronic.zeus.castor.*;
import com.meviatronic.zeus.helen.*;
import org.xiph.ogg.*;

import java.io.*;
import java.awt.*;
import java.awt.image.*;

/**
 * The <code>VideoReader</code> class provides all necessary video format detection related methods.
 * The <code>VideoReader</code> class stors also video header related data.
 *
 * @author	Michael Scheerer
 */
public final class VideoReader extends AudioReader {
	
	private final static byte ZIG_ZAG_SCAN[] = {
		 0,  1,  5,  6, 14, 15, 27, 28,
		 2,  4,  7, 13, 16, 26, 29, 42,
		 3,  8, 12, 17, 25, 30, 41, 43,
		 9, 11, 18, 24, 31, 40, 44, 53,
		10, 19, 23, 32, 39, 45, 52, 54,
		20, 22, 33, 38, 46, 51, 55, 60,
		21, 34, 37, 47, 50, 56, 59, 61,
		35, 36, 48, 49, 57, 58, 62, 63
	};
	
	private static TheoraDecoder dec;
	private static Tag tag;
	
	private static int byteIdx;
	private static byte[] data;
	private static int bitIdx;
	private static int packetByteIdx;
	private static int packetSize;
	
	// Theora header
	static int mbWidth;
	static int mbHeight;
	static int sbChromaWidth;
	static int sbChromaHeight;
	static int sbLumaWidth;
	static int sbLumaHeight;
	public static int codedPictureWidth;
	public static int codedPictureHeight;
	public static int pictureRegionW;
	public static int pictureRegionH;
	public static int pictureRegionX;
	public static int pictureRegionY;
	private static int colorSpace;
	private static int videoBitrate;
	private static int qualityHint;
	static int keyFrameNumberGranuleShift;
	static int chromaFormat;
	public static Dimension aspectRatio;
	public static double frameRate;
	static int macroblocks;
	static int superblocks;
	static int blocks;
	private static int versionRevisionNumber;
	
	private static short[] acScale = new short[64]; // A 64-element array of scale
	private static short[] dcScale = new short[64];
	static short [][][][] qmat = new short[2][3][64][64]; // A 64-element array of quantization values for each DCT coefficient in natural order.
	
	private static short[][] bms; // A NBMS/64 array containing the base matrices.
	private static short[][] nqrs = new short[2][3]; // A 2/3 array containing the number of quant
	// ranges for a given qti and pli, respectively. This is at most 63.
	private static short[][][] qrSizes = new short[2][3][63]; // A 2/3/63 array of the sizes of
	// each quant range for a given qti and pli, respectively.
	// Only the first NQRS[qti ][pli] values are used.
	private static short[][][] qrbmis = new short[2][3][64]; // A 2/3/64 array of the
	// bmi’s used for each quant range for a given qti and pli, respectively.
	// Only the first (NQRS[qti ][pli] + 1) values used
	private static HuffTreeEntry[] hts = new HuffTreeEntry[80]; // An 80-element array of Huffman tables
	// with up to 32 entries each.
	
	static byte[] lflims = new byte[64];
	
	private static boolean headerInitialized1;
	private static boolean headerInitialized2;
	private static boolean headerInitialized3;
	
	private static int entries;
	
	public void loadPacket(byte[] buf, int start, int bytes){
		byteIdx = start;
		data = buf;
		bitIdx = BYTELENGTH;
		packetByteIdx = 0;
		packetSize = bytes;
	}

	private void verifyFirstPacket() throws IOException, EndOfPacketException {
		// Identification header type 0x80, comment header type 0x81, setup header type 0x82.
		// This distinguishes them from video data packets
		// in which the first bit is unset.
		// packetType = get(8);
		
		if (get(8) != 3) {
			throw new InterruptedIOException("Wrong major version");
		}
		if (get(8) != 2) {
			throw new InterruptedIOException("Wrong minor version");
		}
		
		versionRevisionNumber = get(8);
		
		codedPictureWidth = (mbWidth = get(16)) << 4;
		codedPictureHeight = (mbHeight = get(16)) << 4;
		pictureRegionW = get(24);
		pictureRegionH = get(24);
		pictureRegionX = get(8);
		// Theora uses a right-handed coordinate system, while applications expect a left-handed one, so:
		pictureRegionY = codedPictureHeight - pictureRegionH - get(8);

		int framerateNumerator = get(32);
		
		int framerateDenominator = get(32);
		
		int aspectRatioNumerator = get(24);
		
		int aspectRatioDenominator = get(24);
		
		colorSpace = get(8);
		videoBitrate = get(24);
		qualityHint = get(6);
		keyFrameNumberGranuleShift = get(5);
		chromaFormat = get(2);
		if (get(3) != 0) {
			throw new InterruptedIOException("Reserved values not zero");
		}

		if (codedPictureWidth * codedPictureHeight > Integer.MAX_VALUE) {
			throw new InterruptedIOException("Resolution overflow - Java cannot handle images with the size 2^40");
		}
		if (codedPictureWidth < pictureRegionW || codedPictureHeight < pictureRegionH) {
			throw new InterruptedIOException("Picture region must be less/equal the coded frame");
		}
		if (mbWidth == 0 || mbHeight == 0) {
			throw new InterruptedIOException("Wrong frame size in macroblocks");
		}
		if (pictureRegionW + pictureRegionX > codedPictureWidth || pictureRegionH + pictureRegionY > codedPictureHeight) {
			throw new InterruptedIOException("Wrong picture region offset in pixels");
		}
		if (framerateNumerator == 0 || framerateDenominator == 0) {
			throw new InterruptedIOException("Wrong frame rate");
		} else {
			frameRate = framerateNumerator / (double) framerateDenominator;
		}
		if (aspectRatioNumerator == 0 || aspectRatioDenominator == 0) {
			aspectRatioNumerator = 1;
			aspectRatioDenominator = 1;
		}
				
		aspectRatio = new Dimension(aspectRatioNumerator, aspectRatioDenominator);
			
		if (chromaFormat == 1) {
			throw new InterruptedIOException("Wrong color space");
		}

		macroblocks = mbWidth * mbHeight;
		sbLumaWidth = (mbWidth + 1) / 2;
		sbLumaHeight = (mbHeight + 1) / 2;
		if (chromaFormat == 0) {
			sbChromaWidth = (mbWidth + 3) / 4;
			sbChromaHeight = (mbHeight + 3) / 4;
			blocks = macroblocks * 6;
		} else if (chromaFormat == 2) {
			sbChromaWidth = (mbWidth + 3) / 4;
			sbChromaHeight = (mbHeight + 1) / 2;
			blocks = macroblocks * 8;
		} else {
			sbChromaWidth = (mbWidth + 1) / 2;
			sbChromaHeight = (mbHeight + 1) / 2;
			blocks = macroblocks * 12;
		}
		superblocks = sbLumaWidth * sbLumaHeight + 2 * sbChromaWidth * sbChromaHeight;
		headerInitialized1 = true;
	}
	
	private final void verifySecondPacket() throws IOException, EndOfPacketException {
		try {
			tag = new OggTag(this, false);
			((OggTag) tag).decode();
		} catch (Exception e) {
			if (e instanceof IOException) {
				throw new InterruptedIOException(e.getMessage());
			}
		}
		headerInitialized2 = true;
	}

	private final void verifyThirdPacket() throws IOException, EndOfPacketException {
		
		// 6.4.1 Loop Filter Limit Table Decode
		
		int nbits = get(3); // The size of values being read in the current table.
		
		int qi; // The quantization index.
		
		for (qi = 0; qi < 64; qi++) {
			lflims[qi] = (byte) get(nbits);
		}
		
		// 6.4.2 Quantization Parameters Decode
		
		int qti; // A quantization type index. See Table 3.1.
		int qtj; // A quantization type index.
		int pli; // A color plane index. See Table 2.1.
		int plj; // A color plane index.
		int ci;  // The DCT coefficient index.
		int bmi; // The base matrix index.
		int qri; // The quant range index.
		int newqr; // Flag that indicates a new set of quant ranges will be defined.
		int rpqr; // Flag that indicates the quant ranges o copy will come from the same color plane.
		int nbms; // The number of base matrices
	
		nbits = get(4) + 1;
		
		for (qi = 0; qi < 64; qi++) {
			acScale[qi] = (short) get(nbits);
		}
		
		nbits = get(4) + 1;
		
		for (qi = 0; qi < 64; qi++) {
			dcScale[qi] = (short) get(nbits);
		}
		
		nbms = get(9) + 1;
		
		if (nbms > 384) {
			throw new InterruptedIOException("Base matrice number to high");
		}
		
		bms = new short[nbms][64];
		short[] bmsPointer;
		
		for (bmi = 0; bmi < nbms; bmi++) {
			bmsPointer = bms[bmi];
			for (ci = 0; ci < 64; ci++) {
				bmsPointer[ci] = (short) get(8);
			}
		}
		
		for (qti = 0; qti < 2; qti++) {
			for (pli = 0; pli < 3; pli++) {
				if (qti > 0 || pli > 0) {
					newqr = get1();
				} else {
					newqr = 1;
				}
				if (newqr == 0) {
					if (qti > 0) {
						rpqr = get1();
					} else {
						rpqr = 0;
					}
					if (rpqr == 1) {
						qtj = qti - 1;
						plj = pli;
					} else {
						qtj = (3 * qti + pli - 1) / 3;
						plj = (pli + 2) % 3;
					}
					nqrs[qti][pli] = nqrs[qtj][plj ];
					qrSizes[qti][pli] = qrSizes[qtj][plj];
					qrbmis[qti][pli] = qrbmis[qtj][plj];
				} else {
					qri = qi = 0;

					int check = get(ilog(nbms - 1));
					
					if (check >= nbms) {
						throw new InterruptedIOException("Quant range size exceeds base matrice number");
					}
					qrbmis[qti][pli][qri] = (short) check;
					
					do {
						qrSizes[qti][pli][qri] = (short) (get(ilog(62 - qi)) + 1);
						qi += qrSizes[qti][pli][qri];
						qri++;
						qrbmis[qti][pli][qri] = (short) get(ilog(nbms - 1));
					} while (qi < 63);
					if (qi > 63) {
						throw new InterruptedIOException("Quantization index exceeds 63");
					}
					nqrs[qti][pli] = (short) qri;
				}
			}
		}

		// 6.4.4 DCT Token Huffman Tables
		
		int hti; // The index of the current Huffman table to use.
		
		HuffTreeEntry entry;
		
		for (hti = 0; hti < 80; hti++) {
			entry = new HuffTreeEntry();
			entries = 0;
			buildTree(entry, 0);
			if (!entry.sparse) {
				deflateTree(entry);
				pruneTree(entry);
			}
			hts[hti] = entry;
		}
		
		// 6.4.3 Computing all 384 Quantization Matrixes
		
		short[][][] qmatOuterPointer;
		short[][] qmatInnerPointer;
		
		for (qti = 0; qti < qmat.length; qti++) {
			qmatOuterPointer = qmat[qti];
			for (pli = 0; pli < qmatOuterPointer.length; pli++) {
				qmatInnerPointer = qmatOuterPointer[pli];
				for (qi = 0; qi < qmatInnerPointer.length; qi++) {
					computeQuantizationMatrix(qmatInnerPointer[qi], qti, pli, qi);
				}
			}
		}
		headerInitialized3 = true;
	}
	// 6.4.3 Computing a Quantization Matrix
	private short[] computeQuantizationMatrix(short[] qmat, int qti, int pli, int qi) {
		int ci; // The DCT coefficient index.
		int bmi; // The base matrix index.
		int bmj; // The base matrix index.
		int qri; // The quant range index.
		int qiStart = 0; // The left end-point of the qi range.
		int qiEnd = 0; // The right end-point of the qi range.
		int qmin; // The minimum quantization value allowed for the current coefficient.
		int qscale; // The current scale value.
		
		for (qri = 0; qri < 63; qri++) {
			qiEnd += qrSizes[qti][pli][qri];
			if (qiEnd >= qi && qiStart <= qi) {
				break;
			}
			qiStart = qiEnd;
		}
		
		short qrSizesValue = qrSizes[qti][pli][qri];
		int bmci; // A value containing the interpolated base matrix.
		
		bmi = qrbmis[qti][pli][qri];
		bmj = qrbmis[qti][pli][qri + 1];
		
		short[] bmsPointer = bms[bmi];
		short[] bmsPointer1 = bms[bmj];
		
		for (ci = 0, qmin = 16; ci < 64; ci++) {
			bmci = (2 * (qiEnd - qi) * bmsPointer[ci] + 2 * (qi - qiStart) * bmsPointer1[ci] + qrSizesValue) / (2 * qrSizesValue);
			if (ci > 0 && qti == 0) {
				qmin = 8;
			} else if (ci == 0 && qti == 1) {
				qmin = 32;
			}
			if (ci ==0) {
				qscale = dcScale[qi];
			} else {
				qscale = acScale[qi];
			}
			qmat[ZIG_ZAG_SCAN[ci]] = (short) Math.max(qmin, Math.min((qscale * bmci / 100) << 2, 4096));
		}
			
		return qmat;
	}
	
	public void readMediaInformation(Packet op) throws IOException, EndOfPacketException {
		
		if(op == null) {
			throw new InterruptedIOException("Packet is null");
		}

		loadPacket(op.packetBase, op.packet, op.bytes);
	
        byte[] buffer = new byte[6];
		
		int packetType = get(8);
		
		for (int i = 0; i < buffer.length; i++) {
			buffer[i] = (byte) get(8);
		}
		
		if (!new String(buffer).equals("theora")) {
			throw new InterruptedIOException("No first packet");
		}
	    
		if (packetType == 0x80 && op.bos != 0) {
			if(headerInitialized1) {
          		return;
        	}
			verifyFirstPacket();
		} else if (packetType == 0x81 && op.bos == 0) {
			if(headerInitialized2) {
          		return;
        	}
			verifySecondPacket();
		} else if (packetType == 0x82 && op.bos == 0) {
			if(headerInitialized3) {
          		return;
        	}
			verifyThirdPacket();
		} else {
			throw new InterruptedIOException("Wrong packet order");
		}
	}

	public TheoraDecoder initializeTheoraDecoder() {
		if (dec == null) {
			dec = new TheoraDecoder(this);
		}
		dec.granulepos = -1;
		return dec;
	}
	
	public void forceReinitialization() {
		if (dec != null) {
			dec.close();
			close();
			dec = null;
		}
		headerInitialized1 = headerInitialized2 = headerInitialized3 = false;
	}
	
	int get1() throws IOException, EndOfPacketException {
		if (packetSize == 0) {
			throw new EndOfPacketException();
		}
		if (packetByteIdx >= packetSize && packetSize > 0) {
			throw new InterruptedIOException("Illegal truncate packet decoding mode");
		}

		bitIdx--;
		
		int val = data[byteIdx] >>> bitIdx & 1;

		if (bitIdx == 0) {
			bitIdx = BYTELENGTH;
			byteIdx++;
			packetByteIdx++;
		}
		return val;
	}
	
	/**
	 * Returns an integer with the length i
	 *
	 * @return                               the integer value
	 * @param i                              the length in bits
	 * @exception IOException                if the bits can't be retrieved
	 * @exception EndOfPacketExeption        if an end of packet occur
	 */
	public int get(int i) throws IOException, EndOfPacketException {
		if (i <= 0) {
			return 0;
		}
		if (packetSize == 0) {
			throw new EndOfPacketException();
		}
		if (packetByteIdx >= packetSize && packetSize > 0) {
			throw new InterruptedIOException("Illegal truncate packet decoding mode");
		}

		int val = 0;
		
		int prefix = data[byteIdx] & BITMASK[bitIdx];
		
		bitIdx -= i;
		
		if (bitIdx > 0) {
			return prefix >>> bitIdx;
		} else {
			bitIdx += BYTELENGTH;
			byteIdx++;
			packetByteIdx++;
			if (bitIdx < BYTELENGTH) {
				if (packetByteIdx >= packetSize) {
					throw new InterruptedIOException("Illegal truncate packet decoding mode");
				}
				val = data[byteIdx] & 0xFF;
			}
			if (bitIdx <= 0) {
				bitIdx += BYTELENGTH;
				val <<= BYTELENGTH;
				prefix <<= BYTELENGTH;
				byteIdx++;
				packetByteIdx++;
				if (bitIdx < BYTELENGTH) {
					if (packetByteIdx >= packetSize) {
						throw new InterruptedIOException("Illegal truncate packet decoding mode");
					}
					val |= data[byteIdx] & 0xFF;
				}
				if (bitIdx <= 0) {
					bitIdx += BYTELENGTH;
					val <<= BYTELENGTH;
					prefix <<= BYTELENGTH;
					byteIdx++;
					packetByteIdx++;
					if (bitIdx < BYTELENGTH) {
						if (packetByteIdx >= packetSize) {
							throw new InterruptedIOException("Illegal truncate packet decoding mode");
						}
						val |= data[byteIdx] & 0xFF;
					}
					if (bitIdx <= 0) {
						bitIdx += BYTELENGTH;
						val <<= BYTELENGTH;
						prefix <<= BYTELENGTH;
						byteIdx++;
						packetByteIdx++;
						if (bitIdx < BYTELENGTH) {
							if (packetByteIdx >= packetSize) {
								throw new InterruptedIOException("Illegal truncate packet decoding mode");
							}
							val |= data[byteIdx] & 0xFF;
						}
					}
				}
			}
		}
		val >>>= bitIdx;
		val |= prefix << BYTELENGTH - bitIdx;
		return val;
	}

	public String getOggCommentContent() {
		return tag.toString();
	}

	final int getCodeWord(int bookNumber) throws IOException, EndOfPacketException {
		HuffTreeEntry node = hts[bookNumber];
		
		if (node.sparse) {
			return node.value;
		}
		while (node.value == -1){
			node = node.childFeed[get(node.feed)];
			if (node == null) {
				return -1;
			}
		}
		return node.value;
	}
	
	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		super.close();
		int i, j, k;
		
		if (hts[0] != null) {
			for (i = 0; i < 80; i++) {
				closeTree(hts[i]);
				hts[i] = null;
			}
		} else {
			return;
		}
		data = null;
		for (i = 0; i < bms.length; i++) {
			bms[i] = null;
		}
		bms = null;
	}

	private void deflateTree(HuffTreeEntry node) {
		int i, j, k, feedMinusOne, feedMinusOneMinusJ;
		HuffTreeEntry nodeBase = node;
			
		for (node.feed = 2; node.feed < 33; node.feed++) {
			k = 1 << node.feed;
			HuffTreeEntry[] copy = new HuffTreeEntry[k];
			
			feedMinusOne = node.feed - 1;
			for (i = 0; i < k; i++) {
				for (j = 0; j < node.feed; j++) {
					feedMinusOneMinusJ = feedMinusOne - j;
					nodeBase = nodeBase.child[(i & 1 << feedMinusOneMinusJ) >>> feedMinusOneMinusJ];
					if (nodeBase == null) {
						node.feed--;
						copy = null;
						if (node.feed > 1) {
							for (i = 0; i < node.childFeed.length; i++) {
								deflateTree(node.childFeed[i]);
							}
						}
						return;
					}
				}
				copy[i] = nodeBase;
				nodeBase = node;
			}
			for (i = 0; i < node.childFeed.length; i++) {
				node.childFeed[i].dereferenced = true;
			}
			node.childFeed = copy;
		}
	}
	
	private void buildTree(HuffTreeEntry node, int depth) throws IOException, EndOfPacketException {
		if(get1() == 1) {
			if (depth == 0) {
				node.sparse = true;
			}
			if (++entries > 32) {
				throw new InterruptedIOException("Huffmann table entry overflow");
			}
			node.value = get(5);
			return;
		}
		if (++depth > 32) {
			throw new InterruptedIOException("Huffmann table depht overflow");
		}
		buildTree((node.child[0] = new HuffTreeEntry()), depth);
		buildTree((node.child[1] = new HuffTreeEntry()), depth);
	}
	
	private void pruneTree(HuffTreeEntry node) {
		HuffTreeEntry left = node.child[0];
		HuffTreeEntry right = node.child[1];
		
		if (left != null) {
			pruneTree(left);
			pruneTree(right);
		}
		if (node.dereferenced) {
			node.child = null;
			node = null;
		} else {
			if (node.child != node.childFeed) {
				node.child = null;
			}
		}
	}
	
	private void closeTree(HuffTreeEntry node) {
		if (node.sparse) {
			node.child = null;
			node = null;
			return;
		}

		HuffTreeEntry nodeBase;
		
		for (int i = 0; i < node.childFeed.length; i++) {
			nodeBase = node.childFeed[i];
			if (nodeBase != null) {
				closeTree(nodeBase);
				nodeBase = null;
			}
		}
		node.child = null;
		node.childFeed = null;
		node = null;
	}
	
	private class HuffTreeEntry {
		HuffTreeEntry[] child = new HuffTreeEntry[2];
		HuffTreeEntry[] childFeed = child;
		int value = -1;
		boolean sparse;
		byte feed = 1;
		boolean dereferenced;
	}
}


