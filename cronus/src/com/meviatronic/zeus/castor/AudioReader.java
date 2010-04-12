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

import com.meviatronic.zeus.helen.*;
import org.xiph.ogg.*;

import java.io.*;

/**
 * The <code>AudioReader</code> class provides all necessary audio format detection related methods.
 * The <code>AudioReader</code> class stors also audio header related data.
 *
 * @author	Michael Scheerer
 */
public class AudioReader {
	
	public final static int BITMASK[] = {
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

	protected final static int BYTELENGTH = 8;
	protected final static int DOUBLE_BYTELENGTH = 16;
	protected final static int TRIPLE_BYTELENGTH = 24;
	protected final static int QUADRUPEL_BYTELENGTH = 32;

	private final static int VORBIS_CODEBOOK_SYNC_PATTERN = 0x564342;
	
	private static VorbisDecoder dec;
	private static Tag tag;
	
	public static int version;
  	public static int channels;
	public static int sampleRate;
	public static int bitRateMax  ;
  	public static int bitRate;
	public static int bitRateMin;
	
	private static int byteIdx;
	private static byte[] data;
	private static int revBitIdx;
	private static int packetByteIdx;
	private static int packetSize;
	
	// Vorbis extendet header
	private static HuffTreeEntry[] hts;
	private static int[] codebookCodewordLengths;
	
	static int[] codebookDimensions;
	static float[][] valueVector;
	static int[] blocksizes = new int[2];
	static int[] vorbisFloorTypes;
	static int[] floor1Multiplieres;
	static int[][] floor1PartitionClassList;
	static int[][] floor1ClassDimensions;
	static int[][] floor1ClassSubclasses;
	static int[][] floor1ClassMasterbooks;
	static int[][] floor1XList;
	static int floor1MaximumValues;
	static int[][][] floor1SubclassBooks;
	static int[] vorbisResidueTypes;
	static int[] residueMaximumPasses;
	static int[] residueBegin;
	static int[] residueEnd;
	static int[] residuePartitionSize;
	static int[] residueClassifications;
	static int[] residueClassbooks;
	static int[][] residueCascade;
	static int[][][] residueBooks;
	static int[][] vorbisMappingMagnitude;
	static int[][] vorbisMappingAngle;
	static int[][] vorbisMappingMux;
	static int[][] vorbisMappingSubmapFloor;
	static int[][] vorbisMappingSubmapResidue;
	static int[] vorbisModeBlockflag;
	static int[] vorbisModeWindowtype;
	static int[] vorbisModeTransformtype;
	static int[] vorbisModeMapping;
	static int modeNumberBits;
	
	private static boolean vbr;
	
	private static boolean headerInitialized1;
	private static boolean headerInitialized2;
	private static boolean headerInitialized3;
	
	public void loadPacket(byte[] buf, int start, int bytes){
		byteIdx = start;
		data = buf;
		revBitIdx = packetByteIdx = 0;
		packetSize = bytes;
	}
	
  	private void verifyFirstPacket() throws IOException, EndOfPacketException {
		if (getLittleEndian(32) != 0) {
			throw new InterruptedIOException("Vorbis version not 0");
		}
		channels = getLittleEndian(8);
		sampleRate = getLittleEndian(32);
		int bitRateMax = getLittleEndian(32);
		bitRate = getLittleEndian(32);
		int bitRateMin = getLittleEndian(32);
			
		vbr = true;
		if (!(bitRateMax == bitRateMin && bitRateMax  == bitRate)) {
			vbr = false;
		}
		blocksizes[0] = getLittleEndian(4);
		blocksizes[1] = getLittleEndian(4);
			
		if (blocksizes[0] == 0 || blocksizes[1] == 0 || blocksizes[0] > 13 || blocksizes[1] > 13 || blocksizes[1] < blocksizes[0]) {
			throw new InterruptedIOException("Wrong block size");
		}
		blocksizes[0] = 1 << blocksizes[0];
		blocksizes[1] = 1 << blocksizes[1];
		if (getLittleEndian1() == 0) {
			throw new InterruptedIOException("Wrong framing bit");
		}
		if (channels == 0 || channels > 2) {
			throw new InterruptedIOException("Wrong number of channels");
		}
		if (sampleRate > 48000 || sampleRate == 0) {
			throw new InterruptedIOException("Wrong sample rate");
		}
		if (bitRate == 0) {
			if (bitRateMax > 0 & bitRateMin > 0) {
				bitRate = (bitRateMax + bitRateMin) / 2;
			} else if (bitRateMax > 0) {
				bitRate = bitRateMax;
			} else if (bitRateMax > 0) {
				bitRate = bitRateMin;
			}
		}
		headerInitialized1 = true;
  	}
	
	private final void verifySecondPacket() throws IOException, EndOfPacketException {
		try {
			tag = new OggTag(this, true);
			((OggTag) tag).decode();
		} catch (Exception e) {
			if (e instanceof IOException) {
				throw new InterruptedIOException(e.getMessage());
			}
		}
		headerInitialized2 = true;
	}
	
	private final void verifyThirdPacket() throws IOException, EndOfPacketException {
		int i, j, k;
		
		// 4.2.4.1. Codebooks

		int vorbisCodebookCount = getLittleEndian(8) + 1;
		
		codebookDimensions = new int[vorbisCodebookCount];
		hts = new HuffTreeEntry[vorbisCodebookCount];
		valueVector = new float[vorbisCodebookCount][];
		float[] valueVectorPointer = null;
		
		for (i = 0; i < vorbisCodebookCount; i++) {

			if (getLittleEndian(24) != VORBIS_CODEBOOK_SYNC_PATTERN) {
				throw new InterruptedIOException("Broken codebook sync pattern");
			}
		
			int codebookDimension = getLittleEndian(16);
			codebookDimensions[i] = codebookDimension;
			int codebookEntries = getLittleEndian(24);
			
			int usedEntries = 0;
			int ordered = getLittleEndian1();
			codebookCodewordLengths = new int[codebookEntries];
			int currentLength = 0;
			int maxLength = 0;
			int currentEntry = 0;

			if (ordered == 0) {
				int sparse = getLittleEndian1();
				int flag;
			
				for (j = 0; j < codebookEntries; j++) {
					if(sparse == 1) {
						flag = getLittleEndian1();
						if(flag == 1) {
							currentLength = codebookCodewordLengths[j] = getLittleEndian(5) + 1;
						}
					} else {
						currentLength = codebookCodewordLengths[j] = getLittleEndian(5) + 1;
					}
					if (currentLength > maxLength) {
						maxLength = currentLength;
					}
				}
			} else {
				currentLength = getLittleEndian(5) + 1;
				int number;
				int end;
			
				if (currentLength > maxLength) {
					maxLength = currentLength;
				}
				do {
					number = getLittleEndian(ilog(codebookEntries - currentEntry));
					end = currentEntry + number;
			
					for (j = currentEntry; j < end; j++) {
						codebookCodewordLengths[j] = currentLength;
					}
					currentEntry = number + currentEntry;
					currentLength++;
					if (currentEntry > codebookEntries) {
						throw new InterruptedIOException("Codebook overflow");
					}
					if (currentLength > maxLength) {
						maxLength = currentLength;
					}
				} while (currentEntry < codebookEntries);
			}
			
			for (j = 0; j < codebookEntries; j++) {
				if(codebookCodewordLengths[j] > 0) {
					usedEntries++;
					currentEntry = j;
				}
			}

			HuffTreeEntry node = null;
			
			if (usedEntries == 1) {
				node = new HuffTreeEntry();
				
				node.sparse = true;
				node.value = currentEntry;
			} else if (usedEntries > 1) {
				node = new HuffTreeEntry();
				
				buildTree(node, maxLength);
				deflateTree(node);
				pruneTree(node);
			}

			hts[i] = node;
			
			int codebookLookupType = getLittleEndian(4);
			
			if (codebookLookupType > 2) {
				throw new InterruptedIOException("Code lookup type overflow");
			} else if (codebookLookupType == 0) {
				// Lookup type zero indicates no lookup to be read. Proceed past lookup decode.
				if (valueVectorPointer == null) {
					valueVector[i] = new float[codebookDimension * codebookEntries];
				} else {
					valueVector[i] = valueVectorPointer;
				}
				continue;
			}
			
			float codebookMinimumValue = float32Unpack(getLittleEndian(32));
			float codebookDeltaValue = float32Unpack(getLittleEndian(32));
			int codebookValueBits = getLittleEndian(4) + 1;
			int codebookSequenceP = getLittleEndian1();
			int codebookLookupValues = 0;
			
			if (codebookLookupType == 1) {
				codebookLookupValues = (int) Math.floor(Math.pow(codebookEntries, 1F / codebookDimension));
			} else {
				codebookLookupValues = codebookEntries * codebookDimension;
			}
			
			int[] codebookMultiplicands = new int[codebookLookupValues];
			
			for (j = 0; j < codebookLookupValues; j++) {
				codebookMultiplicands[j] = getLittleEndian(codebookValueBits);
			}
			
			if (usedEntries == 0) {
				continue;
			}

			float last;
			int indexDivisor;
			int multiplicandOffset;
			valueVectorPointer = new float[codebookDimension * codebookEntries];
			float valueVectorContent = 0;
			
			if (codebookLookupType == 1) {
				for (j = 0; j < codebookEntries; j++) {
					last = 0;
					indexDivisor = 1;
				
					for (k = 0; k < codebookDimension; k++) {
						multiplicandOffset = (j / indexDivisor) % codebookLookupValues;
						valueVectorContent = codebookMultiplicands[multiplicandOffset] * codebookDeltaValue + codebookMinimumValue + last;
						if (codebookSequenceP == 1) {
							last = valueVectorContent;
						}
						valueVectorPointer[j * codebookDimension + k] = valueVectorContent;
						indexDivisor *= codebookLookupValues;
					}
				}
			} else {
				for (j = 0; j < codebookEntries; j++) {
					last = 0;
					multiplicandOffset = j * codebookDimension;
				
					for (k = 0; k < codebookDimension; k++) {
						valueVectorContent = codebookMultiplicands[multiplicandOffset + k] * codebookDeltaValue + codebookMinimumValue + last;

						if (codebookSequenceP == 1) {
							last = valueVectorContent;
						}
						valueVectorPointer[multiplicandOffset + k] = valueVectorContent;
					}
				}
			}
			valueVector[i] = valueVectorPointer;
		}
		
		codebookCodewordLengths = null;
		
		// 4.2.4.2. Time domain transforms

		int vorbisTimeCount = getLittleEndian(6) + 1;
		
		for (i = 0; i < vorbisTimeCount; i++) {
			if (getLittleEndian(16) != 0) {
				throw new IOException("Unexcepted end of time domain setup");
			}
		}
		
		// 4.2.4.3. Floors
		
		int vorbisFloorCount = getLittleEndian(6) + 1;
	
		vorbisFloorTypes = new int[vorbisFloorCount];
		int vorbisFloorTyp;
		int floor1Partitions;
		int maximumClass;
		int floor1PartitionClass;
		int floor1ClassSubclass;
		int floor1SubclassRange;
		int floor1Multiplier;
		int rangebits;
		int floor1Values;
		int floor1ClassDimensionsElement;
		floor1Multiplieres = new int[vorbisFloorCount];
		floor1PartitionClassList = new int[vorbisFloorCount][];
		floor1ClassDimensions = new int[vorbisFloorCount][];
		floor1ClassSubclasses = new int[vorbisFloorCount][];
		floor1ClassMasterbooks = new int[vorbisFloorCount][];
		floor1SubclassBooks = new int[vorbisFloorCount][][];
		floor1XList = new int[vorbisFloorCount][];
		
		for (i = 0; i < vorbisFloorCount; i++) {
			vorbisFloorTyp = getLittleEndian(16);
			if (vorbisFloorTyp == 0 || vorbisFloorTyp > 1) {
				throw new IOException("No support of floor type zero or wrong floor type");
			}
			floor1Partitions = getLittleEndian(5);
			floor1PartitionClassList[i] = new int[floor1Partitions];
			maximumClass = -1;
			
			for (j = 0; j < floor1Partitions; j++) {
				floor1PartitionClass = getLittleEndian(4);
				if (floor1PartitionClass > maximumClass) {
					maximumClass = floor1PartitionClass;
				}
				floor1PartitionClassList[i][j] = floor1PartitionClass;
			}

			int[] dimensions = floor1ClassDimensions[i] = new int[maximumClass + 1];
			int[] subclasses = floor1ClassSubclasses[i] = new int[maximumClass + 1];
			int[] masterbooks = floor1ClassMasterbooks[i] = new int[maximumClass + 1];
			int[][] subclassbooks = floor1SubclassBooks[i] = new int[maximumClass + 1][];
			
			for (j = 0; j < maximumClass + 1; j++) {
				dimensions[j] = getLittleEndian(3) + 1;
				floor1ClassSubclass = getLittleEndian(2);
				
				if (floor1ClassSubclass != 0) {
					if ((masterbooks[j] = getLittleEndian(8)) >= vorbisCodebookCount) {
						throw new IOException("Number of classbooks can't be GE than number of huffman books");
					}
				}
				subclasses[j] = floor1ClassSubclass;
				
				floor1SubclassRange = 1 << floor1ClassSubclass;
				
				subclassbooks[j] = new int[floor1SubclassRange];
				
				for (k = 0; k < floor1SubclassRange; k++) {
					if ((subclassbooks[j][k] = getLittleEndian(8) - 1) >= vorbisCodebookCount) {
						throw new IOException("Number of floor subclass books can't be GE than number of huffman books");
					}
				}
			}

			floor1Multiplier = getLittleEndian(2) + 1;
			rangebits = getLittleEndian(4);
			floor1Values = 2;
			
			int[] floor1ClassDimensionsElementTable = new int[floor1Partitions];
			
			for (j = 0; j < floor1Partitions; j++) {
				floor1ClassDimensionsElementTable[j] = floor1ClassDimensionsElement = floor1ClassDimensions[i][floor1PartitionClassList[i][j]];
				floor1Values += floor1ClassDimensionsElement;
			}

			floor1XList[i] = new int[floor1Values];
			if (floor1Values > floor1MaximumValues) {
				floor1MaximumValues = floor1Values;
			}
			floor1XList[i][0] = 0;
			floor1XList[i][1] = 1 << rangebits;
			floor1Values = 2;
			
			for (j = 0; j < floor1Partitions; j++) {
				floor1ClassDimensionsElement = floor1ClassDimensionsElementTable[j];
				
				for (k = 0; k < floor1ClassDimensionsElement; k++) {
					if ((floor1XList[i][floor1Values] = getLittleEndian(rangebits)) >= 1 << rangebits) {
						throw new IOException("Floor book values can't be GE than 1 << rangebits");
					}
					floor1Values++;
				}
			}
			vorbisFloorTypes[i] = vorbisFloorTyp;
			floor1Multiplieres[i] = floor1Multiplier;
		}

		// 4.2.4.4. Residues
		
		int vorbisResidueCount = getLittleEndian(6) + 1;
		
		vorbisResidueTypes = new int[vorbisResidueCount];
		residueBegin = new int[vorbisResidueCount];
		residueEnd = new int[vorbisResidueCount];
		residuePartitionSize = new int[vorbisResidueCount];
		residueClassifications = new int[vorbisResidueCount];
		residueClassbooks = new int[vorbisResidueCount];
		residueMaximumPasses = new int[vorbisResidueCount];
		
		int residueCascadeEntry;
		int classifications;
		int typeEntry;
		int maximumPassesEntry;
		int passes;
       	
		residueCascade = new int[vorbisResidueCount][];
		residueBooks = new int[vorbisResidueCount][][];
		
		for (i = 0; i < vorbisResidueCount; i++) {
			maximumPassesEntry = 0;
			typeEntry = getLittleEndian(16);
			if (typeEntry > 2) {
				throw new IOException("No support of residue type greater 2");
			} else {
				vorbisResidueTypes[i] = typeEntry;
				residueBegin[i] = getLittleEndian(24);
				residueEnd[i] = getLittleEndian(24);
				residuePartitionSize[i] = getLittleEndian(24) + 1;
				classifications = residueClassifications[i] = getLittleEndian(6) + 1;
				
				if ((residueClassbooks[i] = getLittleEndian(8)) >= vorbisCodebookCount) {
					throw new IOException("Number of residue class books can't be greater than number of huffman books");
				}

				int[] cascade = residueCascade[i] = new int[classifications];
				int[][] book = residueBooks[i] = new int[classifications][];
				
				for (j = 0; j < classifications; j++) {
					residueCascadeEntry = getLittleEndian(3);
					if (getLittleEndian1() == 1) {
       					residueCascadeEntry |= getLittleEndian(5) << 3;
					}
					passes = ilog(residueCascadeEntry);
					if (maximumPassesEntry < passes) {
						maximumPassesEntry = passes;
					}
					book[j] = new int[passes];
					cascade[j] = residueCascadeEntry;
				}

				residueMaximumPasses[i] = maximumPassesEntry;;

				for (j = 0; j < classifications; j++) {
					for (k = 0; k < book[j].length; k++) {
						if ((cascade[j] & 1 << k) != 0) {
							if ((book[j][k] = getLittleEndian(8)) >= vorbisCodebookCount) {
								throw new IOException("Number of residue books can't be greater than number of huffman books");
							}
						}
					}
				}
			}
		}
		
		// 4.2.4.5. Mappings
		
		int vorbisMappingCount = getLittleEndian(6) + 1;
		int vorbisMappingSubmaps;
		int vorbisMappingSubmapsMax = -1;
		int vorbisMappingCouplingSteps;
		vorbisMappingMagnitude = new int[vorbisMappingCount][];
		vorbisMappingAngle = new int[vorbisMappingCount][];
		vorbisMappingMux = new int[vorbisMappingCount][channels];
		vorbisMappingSubmapFloor = new int[vorbisMappingCount][];
		vorbisMappingSubmapResidue = new int[vorbisMappingCount][];
		int vorbisMappingMagnitudeEntry;
		int vorbisMappingAngleEntry;
		
		for (i = 0; i < vorbisMappingCount; i++) {
			if (getLittleEndian(16) != 0) {
				throw new IOException("Wrong mapping type");
			}
			if (getLittleEndian1() == 1) {
				vorbisMappingSubmaps = getLittleEndian(4) + 1;
			} else {
				vorbisMappingSubmaps = 1;
			}
			if (vorbisMappingSubmaps > vorbisMappingSubmapsMax) {
				vorbisMappingSubmapsMax = vorbisMappingSubmaps;
			}
			if (getLittleEndian1() == 1) {
				vorbisMappingCouplingSteps = getLittleEndian(8) + 1;
				vorbisMappingMagnitude[i] = new int[vorbisMappingCouplingSteps];
				vorbisMappingAngle[i] = new int[vorbisMappingCouplingSteps];
				
				for (j = 0; j < vorbisMappingCouplingSteps; j++) {
					vorbisMappingMagnitudeEntry = getLittleEndian(ilog(channels - 1));
					vorbisMappingAngleEntry = getLittleEndian(ilog(channels - 1));
					if (vorbisMappingMagnitudeEntry == vorbisMappingAngleEntry || vorbisMappingMagnitudeEntry >= channels || vorbisMappingAngleEntry >= channels) {
						throw new IOException("Wrong channel mapping");
					}
					
					vorbisMappingMagnitude[i][j] = vorbisMappingMagnitudeEntry;
					vorbisMappingAngle[i][j] = vorbisMappingAngleEntry;
				}
			} else {
				vorbisMappingCouplingSteps = 0;
			}
			if (getLittleEndian(2) != 0) {
				throw new IOException("Reserved bits should be zero");
			}
			if (vorbisMappingSubmaps > 1) {
				for (j = 0; j < channels; j++) {
					if ((vorbisMappingMux[i][j] = getLittleEndian(4)) >= vorbisMappingSubmapsMax) {
						throw new IOException("Wrong channel mapping mux");
					}
				}
			}
			
			int[] floor = vorbisMappingSubmapFloor[i] = new int[vorbisMappingSubmaps];
			int[] residue = vorbisMappingSubmapResidue[i] = new int[vorbisMappingSubmaps];
			
			for (j = 0; j < vorbisMappingSubmaps; j++) {
				skipLittleEndian(8);
				if ((floor[j] = getLittleEndian(8)) >= vorbisFloorCount) {
					throw new IOException("Wrong floor mapping number");
				}
				if ((residue[j] = getLittleEndian(8)) >= vorbisResidueCount) {
					throw new IOException("Wrong residue mapping number");
				}
			}
		}
		
		// 4.2.4.6. Modes
		
		int vorbisModeCount = getLittleEndian(6) + 1;
		modeNumberBits = ilog(vorbisModeCount - 1);
		vorbisModeBlockflag = new int[vorbisModeCount];
		vorbisModeWindowtype = new int[vorbisModeCount];
		vorbisModeTransformtype = new int[vorbisModeCount];
		vorbisModeMapping = new int[vorbisModeCount];
		
		for (i = 0; i < vorbisModeCount; i++) {
			vorbisModeBlockflag[i] = getLittleEndian1();
			if ((vorbisModeWindowtype[i] = getLittleEndian(16)) > 0) {
				throw new IOException("Wrong mode window type");
			}
			if ((vorbisModeTransformtype[i] = getLittleEndian(16)) > 0) {
				throw new IOException("Wrong mode transform type");
			}
			if ((vorbisModeMapping[i] = getLittleEndian(8)) >= vorbisMappingCount) {
				throw new IOException("Wrong mode mapping number");
			}
		}
		if (getLittleEndian1() == 0) {
			throw new IOException("Framing bit error");
		}
		headerInitialized3 = true;
	}

	public void readMediaInformation(Packet op) throws IOException, EndOfPacketException {

		if(op == null) {
			throw new InterruptedIOException("Packet is null");
		}

		loadPacket(op.packetBase, op.packet, op.bytes);
		
		byte[] buffer = new byte[6];
		
		int packetType = getLittleEndian(8);
			
		for (int i = 0; i < buffer.length; i++) {
			buffer[i] = (byte) getLittleEndian(8);
		}
		
		if (!new String(buffer).equals("vorbis")) {
			throw new InterruptedIOException("No first packet");
		}
	
		if (packetType == 0x01 && op.bos != 0) {
			verifyFirstPacket();
		} else if (packetType == 0x03 && op.bos == 0) {
			verifySecondPacket();
		} else if (packetType == 0x05 && op.bos == 0) {
			verifyThirdPacket();
		} else {
			throw new InterruptedIOException("Wrong packet order");
		}
	}
	
	public final VorbisDecoder initializeVorbisDecoder() {
		if (dec == null) {
			dec = new VorbisDecoder(this);
		}
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

	/**
	 * Returns one bit
	 *
	 * @return                               the integer value
	 * @exception EndOfPacketExeption        if an end of packet occur
	 */
	public final int getLittleEndian1() throws EndOfPacketException {
		if (packetByteIdx >= packetSize) {
			throw new EndOfPacketException();
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
	
	/**
	 * Returns an integer with the length i
	 *
	 * @return                               the integer value
	 * @param i                              the length in bits
	 * @exception EndOfPacketExeption        if an end of packet occur
	 */
	public final int getLittleEndian(int i) throws EndOfPacketException {
		if (i <= 0) {
			return 0;
		}
		if (packetByteIdx >= packetSize) {
			throw new EndOfPacketException();
		}

		int store = revBitIdx;
		
		int val = (data[byteIdx] & 0xFF) >>> store;
		
		revBitIdx += i;
		
		if (revBitIdx >= BYTELENGTH) {
			byteIdx++;
			if (++packetByteIdx >= packetSize) {
				throw new EndOfPacketException();
			}
			val |= (data[byteIdx] & 0xFF) << BYTELENGTH - store;
			if (revBitIdx >= DOUBLE_BYTELENGTH) {
				byteIdx++;
				if (++packetByteIdx >= packetSize) {
					throw new EndOfPacketException();
				}
				val |= (data[byteIdx] & 0xFF) << DOUBLE_BYTELENGTH - store;
				if (revBitIdx >= TRIPLE_BYTELENGTH) {
					byteIdx++;
					if (++packetByteIdx >= packetSize) {
						throw new EndOfPacketException();
					}
					val |= (data[byteIdx] & 0xFF) << TRIPLE_BYTELENGTH - store;
					if (revBitIdx >= QUADRUPEL_BYTELENGTH) {
						byteIdx++;
						if (++packetByteIdx >= packetSize) {
							throw new EndOfPacketException();
						}
						val |= ((data[byteIdx] & 0xFF) << QUADRUPEL_BYTELENGTH - store) & 0xFF000000;
					}
				}
			}
			revBitIdx &= 7;
		}
		return val & BITMASK[i];
	}
	
	final void skipLittleEndian(int j) throws EndOfPacketException {
		if (packetByteIdx >= packetSize) {
			throw new EndOfPacketException();
		}
		revBitIdx += j;
		while (revBitIdx >= BYTELENGTH) {
			revBitIdx -= BYTELENGTH;
			byteIdx++;
			if (++packetByteIdx >= packetSize) {
				throw new EndOfPacketException();
			}
		}
	}

	public final static int ilog(int v) {
    	int ret = 0;
		
		while (v != 0) {
			ret++;
			v >>>= 1;
		}
		return ret;
	}
	
	private static float float32Unpack(int x) {
		int mantissa = x & 0x1FFFFF;
		int exponent = (x & 0x7FE00000) >>> 21;
		
		if ((x & 0x80000000) != 0) {
			mantissa = -mantissa;
		}
		return (float) (mantissa * Math.pow(2, exponent - 788));
	}

  	public String getOggCommentContent() {
		return tag.toString();
	}
	
	final int getCodeWord(int bookNumber) throws IOException, EndOfPacketException {
		
		HuffTreeEntry node = hts[bookNumber];
		
		if (node == null) {
			throw new IOException("Unexcepted end of codebook");
		}
		if (node.sparse) {
			return node.value;
		}
		while (node.value == -1) {
			node = node.childFeed[getLittleEndian(node.feed)];
			if (node == null) {
				throw new IOException("Unexcepted end of codebook");
			}
		}
		return node.value;
	}

	final boolean bookIsUnused(int bookNumber) {
		return hts[bookNumber] == null ? true : false;
	}

	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		int i, j;
		
		if (hts != null) {
			for (i = 0; i < hts.length; i++) {
				if (hts[i] != null) {
					closeTree(hts[i]);
				}
				hts[i] = null;
			}
		} else {
			return;
		}
		tag.close();
		tag = null;
		
		hts = null;
		data = null;
		for (i = 0; i < valueVector.length; i++) {
			valueVector[i] = null;
		}
		valueVector = null;
		codebookDimensions = null;
		vorbisFloorTypes = null;
		floor1Multiplieres = null;
		for (i = 0; i < floor1PartitionClassList.length; i++) {
			floor1PartitionClassList[i] = null;
		}
		floor1PartitionClassList = null;
		for (i = 0; i < floor1ClassDimensions.length; i++) {
			floor1ClassDimensions[i] = null;
		}
		floor1ClassDimensions = null;
		for (i = 0; i < floor1ClassSubclasses.length; i++) {
			floor1ClassSubclasses[i] = null;
		}
		floor1ClassSubclasses = null;
		for (i = 0; i < floor1ClassMasterbooks.length; i++) {
				floor1ClassMasterbooks[i] = null;
		}
		floor1ClassMasterbooks = null;
		for (i = 0; i < floor1XList.length; i++) {
			floor1XList[i] = null;
		}
		floor1XList = null;
		
		int[][] pointer;
		
		for (i = 0; i < floor1SubclassBooks.length; i++) {
			pointer = floor1SubclassBooks[i];
			for (j = 0; j < pointer.length; j++) {
				pointer[j] = null;
			}
			pointer = null;
		}
		floor1SubclassBooks = null;
		vorbisResidueTypes = null;
		residueMaximumPasses = null;
		residueBegin = null;
		residueEnd = null;
		residuePartitionSize = null;
		residueClassifications = null;
		residueClassbooks = null;
		for (i = 0; i < residueCascade.length; i++) {
			residueCascade[i] = null;
		}
		residueCascade = null;
		for (i = 0; i < residueBooks.length; i++) {
			pointer = residueBooks[i];
			for (j = 0; j < pointer.length; j++) {
				pointer[j] = null;
			}
			pointer = null;
		}
		residueBooks = null;
		for (i = 0; i < vorbisMappingMagnitude.length; i++) {
			vorbisMappingMagnitude[i] = null;
		}
		vorbisMappingMagnitude = null;
		for (i = 0; i < vorbisMappingAngle.length; i++) {
			vorbisMappingAngle[i] = null;
		}
		vorbisMappingAngle = null;
		for (i = 0; i < vorbisMappingMux.length; i++) {
			vorbisMappingMux[i] = null;
		}
		vorbisMappingMux = null;
		for (i = 0; i < vorbisMappingSubmapFloor.length; i++) {
			vorbisMappingSubmapFloor[i] = null;
		}
		vorbisMappingSubmapFloor = null;
		for (i = 0; i < vorbisMappingSubmapResidue.length; i++) {
			vorbisMappingSubmapResidue[i] = null;
		}
		vorbisMappingSubmapResidue = null;
		vorbisModeBlockflag = null;
		vorbisModeWindowtype = null;
		vorbisModeTransformtype = null;
		vorbisModeMapping = null;
	}
	
	public int blocksize(Packet op){
    	int modeNumber;
 
    	loadPacket(op.packetBase, op.packet, op.bytes);
		
		try {
			if (getLittleEndian1() != 0) {
				return -1;
			}
		
			modeNumber = getLittleEndian(modeNumberBits);
			
		} catch (Exception e) {
			return -1;
		}
		return blocksizes[vorbisModeBlockflag[modeNumber]];
	}

	private void deflateTree(HuffTreeEntry node) throws IOException {
		int i, j, k, l, r, feedMinusOne, feedMinusOneMinusJ;
		HuffTreeEntry nodeBase = node, nodeVerify;
			
		for (node.feed = 2; node.feed < 33; node.feed++) {
			k = 1 << node.feed;
			HuffTreeEntry[] copy = new HuffTreeEntry[k];
			
			feedMinusOne = node.feed - 1;
			for (i = 0; i < k; i++) {
				for (j = 0; j < node.feed; j++) {
					feedMinusOneMinusJ = feedMinusOne - j;
					l = (i & 1 << feedMinusOneMinusJ) >>> feedMinusOneMinusJ;
					nodeVerify = nodeBase.child[l ^ 1];
					nodeBase = nodeBase.child[l];
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
					if (nodeVerify == null) {
						throw new InterruptedIOException("Underpopulated tree");
					}
				}
				r = 0;
				for (j = 0; j < node.feed; j++) {
					r <<= 1;
					r |= i >>> j & 1;
				}
				copy[r] = nodeBase;
				nodeBase = node;
			}
			for (i = 0; i < node.childFeed.length; i++) {
				node.childFeed[i].dereferenced = true;
			}
			node.childFeed = copy;
		}
	}
	
	private void buildTree(HuffTreeEntry node, int maxLength) throws IOException {
		int i, j;
		int shiftedNodeCount;
		int limit = maxLength + 1;
		int[] shiftedNodeCounts = new int[limit];
		int currentLength;
		int currentValue;
		HuffTreeEntry nodeBase = node;
			
		for (i = 0; i < codebookCodewordLengths.length; i++) {
			if ((currentLength = codebookCodewordLengths[i]) == 0) {
				continue;
			}
			currentValue = shiftedNodeCount = shiftedNodeCounts[currentLength];
			if (shiftedNodeCount >>> currentLength != 0) {
				throw new InterruptedIOException("Overpopulated tree");
			}
			for (j = currentLength; j > 0; j--) {
				if ((shiftedNodeCounts[j] & 1) != 0) {
					if (j == 1) {
						shiftedNodeCounts[1]++;
					} else {
						shiftedNodeCounts[j] = shiftedNodeCounts[j - 1] << 1;
					}
					break;
				}
				shiftedNodeCounts[j]++;
			}
			for (j = currentLength + 1; j < limit; j++) {
				if ((shiftedNodeCounts[j] >>> 1) == shiftedNodeCount) {
					shiftedNodeCount = shiftedNodeCounts[j];
					shiftedNodeCounts[j] = shiftedNodeCounts[j - 1] << 1;
				} else {
					break;
				}
			}
			for (j = currentLength - 1; j >= 0; j--) {
				if ((currentValue >>> j & 0x1) == 1) {
					if (nodeBase.child[1] == null) {
						nodeBase.child[1] = new HuffTreeEntry();
					}
					nodeBase = nodeBase.child[1];
				} else {
					if (nodeBase.child[0] == null) {
						nodeBase.child[0] = new HuffTreeEntry();
					}
					nodeBase = nodeBase.child[0];
				}
			}
			nodeBase.value = i;
			nodeBase = node;
		}
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
		} else if (node.child != node.childFeed) {
			node.child = null;
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


