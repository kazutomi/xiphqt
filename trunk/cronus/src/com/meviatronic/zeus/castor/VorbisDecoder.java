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

import java.io.*;

/**
 * The <code>VorbisDecoder</code> class handles the main decoding of all vorbis formats.
 *
 * @author	Michael Scheerer
 */
public final class VorbisDecoder extends Output {
	
	private final static int FLOOR_MULTIPLIER[] = {
		256, 128, 86, 64
	};
	
	private final static float FLOOR1_INVERSE_DB_TABLE[] = {
		1.0649863e-07F, 1.1341951e-07F, 1.2079015e-07F, 1.2863978e-07F,
		1.3699951e-07F, 1.4590251e-07F, 1.5538408e-07F, 1.6548181e-07F,
		1.7623575e-07F, 1.8768855e-07F, 1.9988561e-07F, 2.128753e-07F,
		2.2670913e-07F, 2.4144197e-07F, 2.5713223e-07F, 2.7384213e-07F,
		2.9163793e-07F, 3.1059021e-07F, 3.3077411e-07F, 3.5226968e-07F,
		3.7516214e-07F, 3.9954229e-07F, 4.2550680e-07F, 4.5315863e-07F,
		4.8260743e-07F, 5.1396998e-07F, 5.4737065e-07F, 5.8294187e-07F,
		6.2082472e-07F, 6.6116941e-07F, 7.0413592e-07F, 7.4989464e-07F,
		7.9862701e-07F, 8.5052630e-07F, 9.0579828e-07F, 9.6466216e-07F,
		1.0273513e-06F, 1.0941144e-06F, 1.1652161e-06F, 1.2409384e-06F,
		1.3215816e-06F, 1.4074654e-06F, 1.4989305e-06F, 1.5963394e-06F,
		1.7000785e-06F, 1.8105592e-06F, 1.9282195e-06F, 2.0535261e-06F,
		2.1869758e-06F, 2.3290978e-06F, 2.4804557e-06F, 2.6416497e-06F,
		2.8133190e-06F, 2.9961443e-06F, 3.1908506e-06F, 3.3982101e-06F,
		3.6190449e-06F, 3.8542308e-06F, 4.1047004e-06F, 4.3714470e-06F,
		4.6555282e-06F, 4.9580707e-06F, 5.2802740e-06F, 5.6234160e-06F,
		5.9888572e-06F, 6.3780469e-06F, 6.7925283e-06F, 7.2339451e-06F,
		7.7040476e-06F, 8.2047000e-06F, 8.7378876e-06F, 9.3057248e-06F,
		9.9104632e-06F, 1.0554501e-05F, 1.1240392e-05F, 1.1970856e-05F,
		1.2748789e-05F, 1.3577278e-05F, 1.4459606e-05F, 1.5399272e-05F,
		1.6400004e-05F, 1.7465768e-05F, 1.8600792e-05F, 1.9809576e-05F,
		2.1096914e-05F, 2.2467911e-05F, 2.3928002e-05F, 2.5482978e-05F,
		2.7139006e-05F, 2.8902651e-05F, 3.0780908e-05F, 3.2781225e-05F,
		3.4911534e-05F, 3.7180282e-05F, 3.9596466e-05F, 4.2169667e-05F,
		4.4910090e-05F, 4.7828601e-05F, 5.0936773e-05F, 5.4246931e-05F,
		5.7772202e-05F, 6.1526565e-05F, 6.5524908e-05F, 6.9783085e-05F,
		7.4317983e-05F, 7.9147585e-05F, 8.4291040e-05F, 8.9768747e-05F,
		9.5602426e-05F, 0.00010181521F, 0.00010843174F, 0.00011547824F,
		0.00012298267F, 0.00013097477F, 0.00013948625F, 0.00014855085F,
		0.00015820453F, 0.00016848555F, 0.00017943469F, 0.00019109536F,
		0.00020351382F, 0.00021673929F, 0.00023082423F, 0.00024582449F,
		0.00026179955F, 0.00027881276F, 0.00029693158F, 0.00031622787F,
		0.00033677814F, 0.00035866388F, 0.00038197188F, 0.00040679456F,
		0.00043323036F, 0.00046138411F, 0.00049136745F, 0.00052329927F,
		0.00055730621F, 0.00059352311F, 0.00063209358F, 0.00067317058F,
		0.00071691700F, 0.00076350630F, 0.00081312324F, 0.00086596457F,
		0.00092223983F, 0.00098217216F, 0.0010459992F, 0.0011139742F,
		0.0011863665F, 0.0012634633F, 0.0013455702F, 0.0014330129F,
		0.0015261382F, 0.0016253153F, 0.0017309374F, 0.0018434235F,
		0.0019632195F, 0.0020908006F, 0.0022266726F, 0.0023713743F,
		0.0025254795F, 0.0026895994F, 0.0028643847F, 0.0030505286F,
		0.0032487691F, 0.0034598925F, 0.0036847358F, 0.0039241906F,
		0.0041792066F, 0.0044507950F, 0.0047400328F, 0.0050480668F,
		0.0053761186F, 0.0057254891F, 0.0060975636F, 0.0064938176F,
		0.0069158225F, 0.0073652516F, 0.0078438871F, 0.0083536271F,
		0.0088964928F, 0.009474637F, 0.010090352F, 0.010746080F,
		0.011444421F, 0.012188144F, 0.012980198F, 0.013823725F,
		0.014722068F, 0.015678791F, 0.016697687F, 0.017782797F,
		0.018938423F, 0.020169149F, 0.021479854F, 0.022875735F,
		0.024362330F, 0.025945531F, 0.027631618F, 0.029427276F,
		0.031339626F, 0.033376252F, 0.035545228F, 0.037855157F,
		0.040315199F, 0.042935108F, 0.045725273F, 0.048696758F,
		0.051861348F, 0.055231591F, 0.058820850F, 0.062643361F,
		0.066714279F, 0.071049749F, 0.075666962F, 0.080584227F,
		0.085821044F, 0.091398179F, 0.097337747F, 0.10366330F,
		0.11039993F, 0.11757434F, 0.12521498F, 0.13335215F,
		0.14201813F, 0.15124727F, 0.16107617F, 0.17154380F,
		0.18269168F, 0.19456402F, 0.20720788F, 0.22067342F,
		0.23501402F, 0.25028656F, 0.26655159F, 0.28387361F,
		0.30232132F, 0.32196786F, 0.34289114F, 0.36517414F,
		0.38890521F, 0.41417847F, 0.44109412F, 0.46975890F,
		0.50028648F, 0.53279791F, 0.56742212F, 0.60429640F,
		0.64356699F, 0.68538959F, 0.72993007F, 0.77736504F,
		0.82788260F, 0.88168307F, 0.9389798F, 1.F
	};
	
	private Imdct[] imdct = {new Imdct(), new Imdct()};
	private float[][] pcm;
	private float[][] previousPcm;
	private float[][] windowTable = new float[5][];
	private float[] window;
	private float[] zeroFloats;
	private int blocksize0, blocksize1, n, previousN;
	private int modeNumber;
	private int modeNumberBits;
	private int vorbisModeBlockflag;
	private int previousVorbisModeBlockflag;
	private int vorbisModeMapping;
	private boolean[] noResidue;
	private float[][] floor;
	private float[][] residue;
	private float[][] residueBundle;
	private Imdct cimdct;
	private int[][] lowNeighborOffsetTable;
	private int[][] highNeighborOffsetTable;
	private int[][] sortTable;
	private int[][][] classificationTable;
	private int[][][] classifications;
	private int[] floor1Step2Flag;
	private int[] floor1FinalY;
	private int[] floor1Y;
	
	/**
	 * Constructs an instance of <code>VorbisDecoder</code> with a
	 * <code>AudioReader</code> object containing all necessary informations
	 * about the media source.
	 *
	 * @param info     the <code>AudioReader</code> object containing all
	 *      necessary informations about the media source
	 */
	public VorbisDecoder(AudioReader info) {
		super(info);
		int i;
		int floor1MaximumValues = info.floor1MaximumValues;
		int floor1XListLength = info.floor1XList.length;
		
		blocksize0 = info.blocksizes[0];
		blocksize1 = info.blocksizes[1];
		modeNumberBits = info.modeNumberBits;
		floor = new float[channels][blocksize1 >>> 1];
		residue = new float[channels][blocksize1 >>> 1];
		residueBundle = new float[channels][];
		
		zeroFloats = new float[blocksize1];
		
		lowNeighborOffsetTable = new int[floor1XListLength][];
		highNeighborOffsetTable = new int[floor1XListLength][];
		sortTable = new int[floor1XListLength][];
		
		floor1Step2Flag = new int[floor1MaximumValues];
		floor1FinalY = new int[floor1MaximumValues];
		floor1Y = new int[floor1MaximumValues];
		
		for (i = 0; i < floor1XListLength; i++) {
			initializeFloorDecoding(i);
		}
		
		classifications = new int[channels][blocksize1 >>> 1][];
		initializeClassification();
		
		pcm = new float[channels][blocksize1];
		noResidue = new boolean[channels];
		previousPcm = new float[channels][blocksize1];
		
		windowTable[0] = new float[blocksize0];
		windowTable[1] = new float[blocksize1];
		windowTable[2] = new float[blocksize1];
		windowTable[3] = new float[blocksize1];
		windowTable[4] = new float[blocksize1];
		
		imdct[0].initialize(blocksize0);
		imdct[1].initialize(blocksize1);
		
		initializeWindowing(blocksize0, 0, 0, 0);
		initializeWindowing(blocksize1, 1, 0, 0);
		initializeWindowing(blocksize1, 1, 0, 1);
		initializeWindowing(blocksize1, 1, 1, 0);
		initializeWindowing(blocksize1, 1, 1, 1);
	}
	
	void decode() throws IOException, EndOfMediaException, EndOfPacketException {
		int i;
		
		if (get1() != 0) {
			throw new InterruptedIOException("No audio packet");
		}
		
		modeNumber = get(modeNumberBits);
		vorbisModeBlockflag = info.vorbisModeBlockflag[modeNumber];
		n = info.blocksizes[vorbisModeBlockflag];
		previousN = info.blocksizes[previousVorbisModeBlockflag];
		
		for (i = 0; i < channels; i++) {
			noResidue[i] = false;
			System.arraycopy(zeroFloats, 0, residue[i], 0, n / 2);
		}
		
		cimdct = imdct[vorbisModeBlockflag];
		
		setBlockSize(previousN + n >>> 2, n);
		
		int previousWindowFlag = 0, nextWindowFlag = 0;
		
		int index = 0;
		
		if (vorbisModeBlockflag == 1) {
			previousWindowFlag = get1();
			nextWindowFlag = get1();
		}
		
		if (previousWindowFlag == 0 && nextWindowFlag == 1) {
			index = 2;
		} else if (previousWindowFlag == 1 && nextWindowFlag == 0) {
			index = 3;
		} else if (previousWindowFlag == 1 && nextWindowFlag == 1) {
			index = 4;
		} else if (vorbisModeBlockflag == 1) {
			index = 1;
		}
	
		window = windowTable[index];
		
		vorbisModeMapping = info.vorbisModeMapping[modeNumber];
		
		boolean noResidues = false;
		
		try {
			for (i = 0; i < channels; i++) {
				decodeFloor(i);
			}
		} catch (EndOfPacketException e) {
			noResidues = true;
		}
		if (!noResidues) {
			nonzeroVectorPropagate();
			try {
				if (decodeResidue()) {
					noResidues = true;
				}
			} catch (EndOfPacketException e) {}
			if (!noResidues) {
				inverseCoupling();
			}
		}
		if (noResidues) {
			for (i = 0; i < channels; i++) {
				float[] p = pcm[i];
				System.arraycopy(zeroFloats, 0, p, 0, p.length);
			}
		} else {
			for (i = 0; i < channels; i++) {
				if (noResidue[i]) {
					float[] p = pcm[i];
					System.arraycopy(zeroFloats, 0, p, 0, p.length);
					continue;
				}
				imdct(residue[i], pcm[i], window, floor[i]);
			}
		}
		postProcessingPcm();
	}
	
	private void decodeFloor(int channel) throws IOException, EndOfMediaException, EndOfPacketException {
		int i, j;
		int submapNumber = info.vorbisMappingMux[vorbisModeMapping][channel];
		int floorNumber = info.vorbisMappingSubmapFloor[vorbisModeMapping][submapNumber];
			
		if (get1() == 0) {
			noResidue[channel] = true;
			return;
		}
			
		int floor1Multiplier = info.floor1Multiplieres[floorNumber];
		int range = FLOOR_MULTIPLIER[floor1Multiplier - 1];
			
		int[] floor1XList = info.floor1XList[floorNumber];
		
		int floor1Values = floor1XList.length;
		
		floor1Y[0] = get(ilog(range - 1));
		floor1Y[1] = get(ilog(range - 1));
			
		int offset = 2;
		int[] floor1PartitionClass = info.floor1PartitionClassList[floorNumber];
		int floor1Partitions = floor1PartitionClass.length;
		int[] floor1ClassDimensions = info.floor1ClassDimensions[floorNumber];
		int[] floor1ClassMasterbooks = info.floor1ClassMasterbooks[floorNumber];
		int[] floor1ClassSubclasses = info.floor1ClassSubclasses[floorNumber];
		int[][] floor1SubclassBooks = info.floor1SubclassBooks[floorNumber];
			
		int pclass;
		int cdim;
		int cbits;
		int csub;
		int cval;
		int book;
		int cdimPlusOffset;
			
		for (i = 0; i < floor1Partitions; i++) {
			pclass = floor1PartitionClass[i];
			cdim  = floor1ClassDimensions[pclass];
			cdimPlusOffset = cdim + offset;
			cbits = floor1ClassSubclasses[pclass];
			csub = (1 << cbits) - 1;
			cval = 0;
			if (cbits > 0) {
				cval = getCodeWord(floor1ClassMasterbooks[pclass]);
			}
			for (j = offset; j < cdimPlusOffset; j++) {
				book = floor1SubclassBooks[pclass][cval & csub];
				cval >>>= cbits;
				if (book >= 0) {
					floor1Y[j] = getCodeWord(book);
				} else {
					floor1Y[j] = 0;
				}
			}
			offset += cdim;
		}
		
		floor1Step2Flag[0] = 1;
		floor1Step2Flag[1] = 1;
		floor1FinalY[0] = floor1Y[0];
		floor1FinalY[1] = floor1Y[1];
			
		int lowNeighborOffset;
		int highNeighborOffset;
		int predicted;
		int val;
		int highroom;
		int lowroom;
		int room;
			
		int[] low = lowNeighborOffsetTable[floorNumber];
		int[] high = highNeighborOffsetTable[floorNumber];
		int[] sort = sortTable[floorNumber];

		for (i = 2; i < floor1Values; i++) {
			lowNeighborOffset = low[i];
			highNeighborOffset = high[i];
			predicted = renderPoint(floor1XList[lowNeighborOffset],
									floor1FinalY[lowNeighborOffset],
									floor1XList[highNeighborOffset],
									floor1FinalY[highNeighborOffset],
									floor1XList[i]);
			
			val = floor1Y[i];
			highroom = range - predicted;
			lowroom = predicted;
				
			room = (highroom < lowroom ? highroom : lowroom) << 1;

			if (val != 0) {
				floor1Step2Flag[lowNeighborOffset] = 1;
				floor1Step2Flag[highNeighborOffset] = 1;
				floor1Step2Flag[i] = 1;
				if (val >= room) {
					if (highroom > lowroom) {
						floor1FinalY[i] = val - lowroom + predicted;
					} else {
						floor1FinalY[i] = predicted - val + highroom - 1;
					}
				} else {
					if ((val & 0x1) == 1) {
						floor1FinalY[i] = predicted - ((val + 1) >>> 1);
					} else {
						floor1FinalY[i] = predicted + (val >>> 1);
					}
				}
			} else {
				floor1Step2Flag[i] = 0;
				floor1FinalY[i] = predicted;
			}
		}
			
		float[] flro = floor[channel];
		int hx = 0;
		int hy = 0;
		int lx = 0;
		int ly = floor1FinalY[0] * floor1Multiplier;
		int actualSize = n / 2;

		for (i = 1; i < floor1Values; i++) {
			j = sort[i];
			if (floor1Step2Flag[j] == 1) {
				hy = floor1FinalY[j] * floor1Multiplier;
				hx = floor1XList[j];
				renderLine(lx, ly, hx, hy, flro);
				lx = hx;
				ly = hy;
			}
		}
		if (hx < actualSize) {
			renderLine(hx, hy, actualSize, hy, flro);
		}
	}
	
	private void nonzeroVectorPropagate() {
		int[] magnitude = info.vorbisMappingMagnitude[vorbisModeMapping];
		int[] angle = info.vorbisMappingAngle[vorbisModeMapping];
		int m;
		int a;
		int i;
		
		for (i = 0; i < magnitude.length; i++) {
			m = magnitude[i];
			a = angle[i];
			if (!noResidue[m] || !noResidue[a]) {
				noResidue[m] = false;
				noResidue[a] = false;
			}
		}
	}

	private boolean decodeResidue() throws IOException, EndOfMediaException, EndOfPacketException {
		int i, j;
		int[] vorbisMappingSubmapResidue = info.vorbisMappingSubmapResidue[vorbisModeMapping];
		int[] vorbisMappingMux = info.vorbisMappingMux[vorbisModeMapping];
		int ch;
		int dch, dch2;
		boolean doNotDecode;
		int residueNumber;
		int residueType;

		for (i = 0; i < vorbisMappingSubmapResidue.length; i++) {
			ch = dch = dch2 = 0;
			residueNumber = vorbisMappingSubmapResidue[i];
			residueType = info.vorbisResidueTypes[residueNumber];
			
			for (j = 0; j < channels; j++) {
				if (vorbisMappingMux[j] == i) {
					if (!(doNotDecode = noResidue[j]) || residueType == 2) {
						residueBundle[dch++] = residue[j];
						if (!doNotDecode) {
							dch2++;
						}
					}
					ch++;
				}
			}
			if (dch2 == 0) {
				return false;
			}
			if (decodeVectors(dch, residueNumber, residueType)) {
				continue;
			}
		}
		return false;
	}
				
	private boolean decodeVectors(int dch, int residueNumber, int residueType) throws IOException, EndOfMediaException, EndOfPacketException {
		int i, j, k, l, pass, entryTemp, end;
		int residueBegin = info.residueBegin[residueNumber];
		int residueEnd = info.residueEnd[residueNumber];
		int residuePartitionSize = info.residuePartitionSize[residueNumber];
		int residueClassifications = info.residueClassifications[residueNumber];
		int residueClassbook = info.residueClassbooks[residueNumber];
		int maximumPasses = info.residueMaximumPasses[residueNumber];
		int[] residueCascade = info.residueCascade[residueNumber];
		int[][] residueBooks = info.residueBooks[residueNumber];
		int actualSize = n / 2;
		int vectors = dch;

		if (residueType == 2) {
			actualSize *= dch;
		}
		
		int limitResidueBegin = residueBegin < actualSize ? residueBegin : actualSize;
		int limitResidueEnd = residueEnd < actualSize ? residueEnd : actualSize;
		int codebookDimensions[] = info.codebookDimensions;
		int codebookDimension;
		int classwordsPerCodeword = codebookDimensions[residueClassbook];
		float[][] vqLookupTable = info.valueVector;
		float[] vqLookup;
		float[] out;

		int nToRead = limitResidueEnd - limitResidueBegin;
		int partitionsToRead = nToRead / residuePartitionSize;
		
		if (nToRead == 0) {
			return true;
		}

		int partitionCount;
		int codewordCount;
		int temp;
		int vqclass;
		int vqbook;
		
		int[][] classificationTablePerResidue = classificationTable[residueNumber];
		
		for (pass = 0; pass < maximumPasses; pass++) {
			partitionCount = 0;
			codewordCount = 0;
			
			if (residueType == 0) {
				while (partitionCount < partitionsToRead) {
					if (pass == 0) {
						for (j = 0; j < vectors; j++) {
							temp = getCodeWord(residueClassbook);
							classifications[j][codewordCount] = classificationTablePerResidue[temp];
						}
					}
					for (i = 0; i < classwordsPerCodeword && partitionCount < partitionsToRead; i++) {
						k = end = limitResidueBegin + partitionCount * residuePartitionSize;
						for (j = 0; j < vectors; j++) {
							vqclass = classifications[j][codewordCount][i];
							out = residueBundle[j];
							if ((residueCascade[vqclass] & 1 << pass) != 0) {
								vqbook = residueBooks[vqclass][pass];
								vqLookup = vqLookupTable[vqbook];
							
								codebookDimension = codebookDimensions[vqbook];
								if(bookIsUnused(vqbook)) {
									continue;
								}
								int step = residuePartitionSize / codebookDimension;
								end += step;

								for (; k < end; k++) {
									entryTemp = getCodeWord(vqbook) * codebookDimension;
									for (l = 0; l < codebookDimension; l++) {
										out[k + l * step] += vqLookup[entryTemp + l];
									}
								}
							}
						}
						partitionCount++;
					}
					codewordCount++;
				}
			} else if (residueType == 1) {
				while (partitionCount < partitionsToRead) {
					if (pass == 0) {
						for (j = 0; j < vectors; j++) {
							temp = getCodeWord(residueClassbook);
							classifications[j][codewordCount] = classificationTablePerResidue[temp];
						}
					}
					for (i = 0; i < classwordsPerCodeword && partitionCount < partitionsToRead; i++) {
						k = end = limitResidueBegin + partitionCount * residuePartitionSize;
							
						for (j = 0; j < vectors; j++) {
							vqclass = classifications[j][codewordCount][i];
							out = residueBundle[j];
							if ((residueCascade[vqclass] & 1 << pass) != 0) {
								vqbook = residueBooks[vqclass][pass];
								vqLookup = vqLookupTable[vqbook];
							
								codebookDimension = codebookDimensions[vqbook];
								if(bookIsUnused(vqbook)) {
									continue;
								}
								end += residuePartitionSize;

								do {
									entryTemp = getCodeWord(vqbook) * codebookDimension;
									for (l = 0; l < codebookDimension; l++, k++) {
										out[k] += vqLookup[entryTemp + l];
									}
								} while (k < end);
							}
						}
						partitionCount++;
					}
					codewordCount++;
				}
			} else {
				while (partitionCount < partitionsToRead) {
					if (pass == 0) {
						temp = getCodeWord(residueClassbook);
						classifications[0][codewordCount] = classificationTablePerResidue[temp];
					}
					for (i = 0; i < classwordsPerCodeword && partitionCount < partitionsToRead; i++) {
						k = end = limitResidueBegin + partitionCount * residuePartitionSize;
						vqclass = classifications[0][codewordCount][i];
						if ((residueCascade[vqclass] & 1 << pass) != 0) {
							vqbook = residueBooks[vqclass][pass];
							vqLookup = vqLookupTable[vqbook];
							
							codebookDimension = codebookDimensions[vqbook];
							if(bookIsUnused(vqbook)) {
								continue;
							}
							end += residuePartitionSize;
							
							if (codebookDimension == 2) {
								do {
									entryTemp = getCodeWord(vqbook) * codebookDimension;
									residueBundle[k % dch][k / dch] += vqLookup[entryTemp];
									k++;
									residueBundle[k % dch][k / dch] += vqLookup[entryTemp + 1];
									k++;
								} while (k < end);
							} else if (codebookDimension == 4) {
								do {
									entryTemp = getCodeWord(vqbook) * codebookDimension;
									residueBundle[k % dch][k / dch] += vqLookup[entryTemp];
									k++;
									residueBundle[k % dch][k / dch] += vqLookup[entryTemp + 1];
									k++;
									residueBundle[k % dch][k / dch] += vqLookup[entryTemp + 2];
									k++;
									residueBundle[k % dch][k / dch] += vqLookup[entryTemp + 3];
									k++;
								} while (k < end);
							} else {
								do {
									entryTemp = getCodeWord(vqbook) * codebookDimension;
									for (l = 0; l < codebookDimension; l++, k++) {
										residueBundle[k % dch][k / dch] += vqLookup[entryTemp + l];
									}
								} while (k < end);
							}
						}
						partitionCount++;
					}
					codewordCount++;
				}
			}
		}
		return false;
	}

	private void inverseCoupling() {
		int[] magnitude = info.vorbisMappingMagnitude[vorbisModeMapping];
		int[] angle = info.vorbisMappingAngle[vorbisModeMapping];
		float[] magnitudeVector;
		float[] angleVector;
		float m, a, newM, newA;
		
		if (magnitude == null) {
			return;
		}

		for (int i = magnitude.length - 1; i >= 0; i--) {
			magnitudeVector = residue[magnitude[i]];
			angleVector = residue[angle[i]];
			
			for (int j = 0; j < magnitudeVector.length; j++) {
				m = magnitudeVector[j];
				a = angleVector[j];
				if (m > 0) {
					if (a > 0) {
						newM = m;
						newA = m - a;
					} else {
						newA = m;
						newM = m + a;
					}
				} else {
					if (a > 0) {
						newM = m;
						newA = m + a;
					} else {
						newA = m;
						newM = m - a;
					}
				}
				magnitudeVector[j] = newM;
				angleVector[j] = newA;
			}
		}
	}
	
	private int lowNeighbor(int[] v, int x) {
		int nMax = 0;
		int treshold = -1;
		int vn, vx;
		
		for (int n = x - 1; n >= 0; n--) {
			vn = v[n];
			vx = v[x];
			
			if (vn > treshold && vn < vx) {
				treshold = vn;
				nMax = n;
			}
		}
		return nMax;
	}
	
	private int highNeighbor(int[] v, int x) {
		int nMin = 0;
		int treshold = Integer.MAX_VALUE;
		int vn, vx;
		
		for (int n = 0; n < x; n++) {
			vn = v[n];
			vx = v[x];
			
			if (vn < treshold && vn > vx) {
				treshold = vn;
				nMin = n;
			}
		}
		return nMin;
	}
	
	private static int renderPoint(int x0, int y0, int x1, int y1, int x) {
		int dy = y1 - y0;
		int adx = x1 - x0;
		int ady = dy;
		
		if (ady < 0) {
			ady = -ady;
		}
		
		int err = ady * (x - x0);
		int off = err / adx;
		
		if (dy < 0) {
			return y0 - off;
		}
		return y0 + off;
	}
	
	private void renderLine(int x0, int y0, int x1, int y1, float[] v) {
		int dy = y1 - y0;
		int adx = x1 - x0;
		int ady = dy;
		
		if (ady < 0) {
			ady = -ady;
		}
		
		int x = x0;
		int y = y0;
		int err = -adx;
		int sy = dy < 0 ? - 1 : + 1;
		int base = dy / adx;
		
		v[x] = FLOOR1_INVERSE_DB_TABLE[y];
			
		if (base == 0) {
			
			if (ady << 1 <= adx) {
				int x1MinusOne = x1 - 1;
				int adyMinusAdx = ady - adx;
					
				while (++x < x1MinusOne) {
					err += ady;
			
					if (err >= 0) {
						err += adyMinusAdx;
						y += sy;
						v[x++] = FLOOR1_INVERSE_DB_TABLE[y];
					}
				
					v[x] = FLOOR1_INVERSE_DB_TABLE[y];
				}
				if (x < x1) {
					if (err + ady >= 0) {
						y += sy;
					}
					v[x] = FLOOR1_INVERSE_DB_TABLE[y];
				}
			} else {
				while (++x < x1) {
					err += ady;
			
					if (err >= 0) {
						err -= adx;
						y += sy;
					}
				
					v[x] = FLOOR1_INVERSE_DB_TABLE[y];
				}
			}
		 } else {
			int abase = base;
		
			if (abase < 0) {
				abase = -abase;
			}

			ady -= abase * adx;
			
			while (++x < x1) {
				err += ady;
			
				if (err >= 0) {
					err -= adx;
					y += sy;
				}
				
				y += base;
				
				v[x] = FLOOR1_INVERSE_DB_TABLE[y];
			}
		}
	}
	
	private void imdct(float[] in, float[] out, float[] window, float[] floor) {
		cimdct.decode(in, out, window, floor);
	}

	private void postProcessingPcm() {
		int i = 0;
		int j, k = 0;
		int nHalf = n >>> 1;
		int previousNhalf = previousN >>> 1;
		int begin = 0;
		int middle = nHalf + previousNhalf >>> 1;
		int end = middle;
		int kOffset = 0;
		float f;
		short w;
		byte b2;
		byte b1;
		int p;
		
		if (previousVorbisModeBlockflag == 1 && vorbisModeBlockflag == 0) {
			begin = previousNhalf - nHalf >>> 1;
		} else if (previousVorbisModeBlockflag == 0 && vorbisModeBlockflag == 1) {
			middle = previousNhalf;
			kOffset = nHalf - previousNhalf >>> 1;
		}
		for (; i < begin; i++) {
			for (j = 0; j < channels; j++) {
				setBuffer(previousPcm[j][i], j);
			}
		}
		for (; i < middle; i++, k++) {
			for (j = 0; j < channels; j++) {
				setBuffer(previousPcm[j][i] + pcm[j][kOffset + k], j);
			}
		}
		for (; i < end; i++, k++) {
			for (j = 0; j < channels; j++) {
				setBuffer(pcm[j][kOffset + k], j);
			}
		}
		for (j = 0; j < channels; j++) {
			System.arraycopy(pcm[j], nHalf, previousPcm[j], 0, nHalf);
		}
		previousVorbisModeBlockflag = vorbisModeBlockflag;
	}

	private void initializeWindowing(int n, int vorbisModeBlockflag, int previousWindowFlag, int nextWindowFlag) {
		int index = 0;
		int windowCenter = n / 2;
		int rightWindowStart, rightWindowEnd, leftWindowStart, leftWindowEnd, leftN, rightN;
		
		if (vorbisModeBlockflag == 1 && previousWindowFlag == 0) {
			leftN = blocksize0 / 2;
			leftWindowStart = n / 4 - leftN / 2;
			leftWindowEnd = n / 4 + leftN / 2;
		} else {
			leftWindowStart = 0;
			leftN = leftWindowEnd = windowCenter;
		}
			
		if (vorbisModeBlockflag == 1 && nextWindowFlag == 0) {
			rightN = blocksize0 / 2;
			rightWindowStart = n * 3 / 4 - rightN / 2;
			rightWindowEnd = n * 3 / 4 + rightN / 2;
		} else {
			rightN = rightWindowStart = windowCenter;
			rightWindowEnd = n;
		}
		
		if (previousWindowFlag == 0 && nextWindowFlag == 1) {
			index = 2;
		} else if (previousWindowFlag == 1 && nextWindowFlag == 0) {
			index = 3;
		} else if (previousWindowFlag == 1 && nextWindowFlag == 1) {
			index = 4;
		} else if (vorbisModeBlockflag == 1) {
			index = 1;
		}

		int i = leftWindowStart;
		
		float[] windowTablePointer = windowTable[index];

		for (; i < leftWindowEnd; i++) {
			windowTablePointer[i] = (float) Math.sin(0.5 * Math.PI * Math.pow(Math.sin((i - leftWindowStart + 0.5) / leftN * 0.5 * Math.PI), 2));
		}
		for (; i < rightWindowStart; i++) {
			windowTablePointer[i] = 1;
		}
		for (; i < rightWindowEnd; i++) {
			windowTablePointer[i] = (float) Math.sin(0.5 * Math.PI * Math.pow(Math.sin((rightN + i - rightWindowStart + 0.5) / rightN * 0.5 * Math.PI), 2));
		}
	}
	
	private void initializeFloorDecoding(int floorNumber) {
		int i, j;
		int[] floor1XList = info.floor1XList[floorNumber];
		int floor1Values = floor1XList.length;
		
		int[] low = lowNeighborOffsetTable[floorNumber] = new int[floor1Values];
		int[] high = highNeighborOffsetTable[floorNumber] = new int[floor1Values];
		int[] sort = sortTable[floorNumber] = new int[floor1Values];
		int buffer;

		for (i = 1; i < floor1Values; i++) {
			sort[i] = i;
		}
		for (i = 1; i < floor1Values; i++) {
			for (j = i + 1; j < floor1Values && i < floor1Values - 1; j++) {
				if (floor1XList[sort[i]] > floor1XList[sort[j]]) {
					buffer = sort[i];
					sort[i] = sort[j];
					sort[j] = buffer;
				}
			}
			low[i] = lowNeighbor(floor1XList, i);
			high[i] = highNeighbor(floor1XList, i);
		}
	}
	
	private void initializeClassification() {
		int i, j, temp;

		classificationTable = new int[info.residueClassifications.length][][];
		int[][] classificationTableOuterPointer;
		int[] classificationTableInnerPointer;

		for (i = 0; i < info.residueClassifications.length; i++) {
			int residueClassifications = info.residueClassifications[i];
			int residueClassbooks = info.residueClassbooks[i];
			int classwordsPerCodeword = info.codebookDimensions[residueClassbooks];
			int maxNumber = (int) Math.pow(residueClassifications, classwordsPerCodeword);
			int workingDigit = 0;
			int tempBuffer;
			int classification;
			classificationTableOuterPointer = classificationTable[i] = new int[maxNumber][];
			
			for(temp = 0; temp < maxNumber; temp++) {
				classificationTableInnerPointer = classificationTableOuterPointer[temp] = new int[classwordsPerCodeword];
      			tempBuffer = temp;
      			workingDigit = maxNumber / residueClassifications;
				
      			for(j = 0; j < classwordsPerCodeword; j++){
					classification = tempBuffer / workingDigit;
					tempBuffer -= classification * workingDigit;
					workingDigit /= residueClassifications;
					classificationTableInnerPointer[j] = classification;
				}
			}
		}
	}

	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		super.close();
		int i, j;
		
		if (pcm != null) {
			for (i = 0; i < pcm.length; i++) {
				pcm[i] = null;
			}
		}
		pcm = null;
		if (previousPcm != null) {
			for (i = 0; i < previousPcm.length; i++) {
				previousPcm[i] = null;
			}
		}
		previousPcm = null;
		
		if (windowTable != null) {
			for (i = 0; i < windowTable.length; i++) {
				windowTable[i] = null;
			}
		}
		windowTable = null;
		
		int[][] classificationTablePointer;
		
		if (classificationTable != null) {
			for (i = 0; i < classificationTable.length; i++) {
				classificationTablePointer = classificationTable[i];
				for (j = 0; j < classificationTablePointer.length; j++) {
					classificationTablePointer[j] = null;
				}
				classificationTablePointer = null;
			}
		}
		classificationTable = null;
		if (classifications != null) {
			for (i = 0; i < classifications.length; i++) {
				classifications[i] = null;
			}
		}
		classifications = null;
		if (floor != null) {
			for (i = 0; i < floor.length; i++) {
				floor[i] = null;
			}
		}
		floor = null;
		residueBundle = null;
		
		if (residue != null) {
			for (i = 0; i < residue.length; i++) {
				residue[i] = null;
			}
		}
		residue = null;
		
		if (lowNeighborOffsetTable != null) {
			for (i = 0; i < lowNeighborOffsetTable.length; i++) {
				lowNeighborOffsetTable[i] = null;
			}
		}
		lowNeighborOffsetTable = null;
		
		if (highNeighborOffsetTable != null) {
			for (i = 0; i < highNeighborOffsetTable.length; i++) {
				highNeighborOffsetTable[i] = null;
			}
		}
		highNeighborOffsetTable = null;

		if (sortTable != null) {
			for (i = 0; i < sortTable.length; i++) {
				sortTable[i] = null;
			}
		}
		sortTable = null;
		floor1Step2Flag = null;
		floor1FinalY = null;
		floor1Y = null;
		noResidue = null;
		zeroFloats = null;
		if (imdct != null) {
			imdct[0].close();
			imdct[1].close();
			imdct[0] = null;
			imdct[1] = null;
		}
		imdct = null;
	}
}
