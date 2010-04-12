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

import java.io.*;

/**
 * The <code>TheoraDecoder</code> class is the main video decoder class.
 *
 * @author  Michael Scheerer
 */
public final class TheoraDecoder extends Output  {
	
	private final static int CLIP_MIN = 33024;
	private final static int CLIP_MAX = 33024;
	private final static int CLIP_RANGE = 66048;
	
	private static short clip[] = new short[CLIP_RANGE];
	
	private final static int FILTER_MIN = 256;
	private final static int FILTER_RANGE = 512;
	
	private final static int PLANE_CLIP_MIN = 16;
	private final static int PLANE_CLIP_MAX = 16;
	private final static int PLANE_CLIP_RANGE = 32;
	
	private final static byte INTER_NOMV = 0;
	private final static byte INTRA = 1;
	private final static byte INTER_MV = 2;
	private final static byte INTER_MV_LAST = 3;
	private final static byte INTER_MV_LAST2 = 4;
	private final static byte INTER_GOLDEN_NOMV = 5;
	private final static byte INTER_GOLDEN_MV = 6;
	private final static byte INTER_MV_FOUR = 7;
	
	private final static byte INTRA_FRAME = 0;
	private final static byte PREVIOUS_REFERENCE_FRAME_INDEX = 1;
	private final static byte GOLDEN_REFERENCE_FRAME_INDEX = 2;
	
	private final static int C1 = 64277;
	private final static int S7 = 64277;
	private final static int C2 = 60547;
	private final static int S6 = 60547;
	private final static int C3 = 54491;
	private final static int S5 = 54491;
	private final static int C4 = 46341;
	private final static int S4 = 46341;
	private final static int C5 = 36410;
	private final static int S3 = 36410;
	private final static int C6 = 25080;
	private final static int S2 = 25080;
	private final static int C7 = 12785;
	private final static int S1 = 12785;
		
	private final static short[] ZERO_SHORTS = new short[64];
	
	private final static byte NATURAL_ORDER_SCAN[] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63,
		63, 63, 63, 63, 63, 63, 63, 63
	};
	private final static byte INVERSE_ZIG_ZAG_SCAN[] = {
		 0,  1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63
	};
	private final static byte ZIG_ZAG_TO_NATURAL_ORDER_ROW_MAPPING[] = {
		 0,  0,  1,  2,  2,  2,  2,  2,
		 2,  3,  4,  4,  4,  4,  4,  4,
		 4,  4,  4,  4,  5,  6,  6,  6,
		 6,  6,  6,  6,  6,  6,  6,  6,
		 6,  6,  6,  7,  7,  7,  7,  7,
		 7,  7,  7,  7,  7,  7,  7,  7,
		 7,  7,  7,  7,  7,  7,  7,  7,
		 7,  7,  7,  7,  7,  7,  7,  7
	};
	// [yRemainder][xRemainder][bi]
	private final static byte SUPERBLOCK_X_BLOCKS_OFFSET[][][] = {
	  {{0},
		   {0, 1},
				  {0, 1, 2},
				  			{0, 1, 2, 3}},
	  {{0,
	    0},
		   {0, 1,
		    1, 0},
				  {0, 1, 1,
				   0, 2, 2},
							{0, 1, 1, 0,
							 3, 2, 2, 3}},
	  {{0,
	    0,
	    0},
		   {0, 1,
		    1, 0,
		    0, 1},
				  {0, 1, 1,
				   0, 0, 1,
				   2, 2, 2},
				  			{0, 1, 1, 0,
							 0, 1, 2, 3,
							 3, 2, 2, 3}},
	  {{0,
	    0,
	    0,
	    0},
		   {0, 1,
			1, 0,
	      	0, 0,
		  	1, 1},
			  	  {0, 1, 1,
				   0, 0, 0,
				   1, 1, 2,
				   2, 2, 2},
							{0, 1, 1, 0,
							 0, 0, 1, 1,
							 2, 2, 3, 3,
							 3, 2, 2, 3}}
	};
	// [yRemainder][xRemainder][bi]
	private final static byte SUPERBLOCK_Y_BLOCKS_OFFSET[][][] = {
	  {{0},
		   {0, 0},
				  {0, 0, 0},
				  			{0, 0, 0, 0}},
	  {{0,
	    1},
		   {0, 0,
		    1, 1},
				  {0, 0, 1,
				   1, 1, 0},
							{0, 0, 1, 1,
							 1, 1, 0, 0}},
	  {{0,
	    1,
	    2},
		   {0, 0,
		    1, 1,
		    2, 2},
				  {0, 0, 1,
				   1, 2, 2,
				   2, 1, 0},
							{0, 0, 1, 1,
							 2, 2, 2, 2,
							 1, 1, 0, 0}},
	  {{0,
	    1,
	    2,
	    3},
		   {0, 0,
		    1, 1,
		    2, 3,
		    3, 2},
				  {0, 0, 1,
				   1, 2, 3,
				   3, 2, 2,
				   3, 1, 0},
							{0, 0, 1, 1,
							 2, 3, 3, 2,
							 2, 3, 3, 2,
							 1, 1, 0, 0}}
	};
	private final static byte HUFFMANCODETABLE7_7RSTART[] = {
		1, 2, 4, 6, 10, 18, 34
	};
	private final static byte HUFFMANCODETABLE7_7RBITS[] = {
		0, 1, 1, 2, 3, 4, 12
	};
	private final static byte HUFFMANCODETABLE7_11RSTART[] = {
		1, 3, 5, 7, 11, 15
	};
	private final static byte HUFFMANCODETABLE7_11RBITS[] = {
		1, 1, 1, 2, 2, 4
	};
	private final static byte HUFFMANCODETABLE7_14[][] = {
		{0, 0, 0, 0, 0, 0, 0, 0},
		{3, 4, 2, 0, 1, 5, 6, 7}, {3, 4, 0, 2, 1, 5, 6, 7},
		{3, 2, 4, 0, 1, 5, 6, 7}, {3, 2, 0, 4, 1, 5, 6, 7},
		{0, 3, 4, 2, 1, 5, 6, 7}, {0, 5, 3, 4, 2, 1, 6, 7}
	};
	// Negative values assign to table columns and the to read bit length,
	// with the exception of -2, where column is 2 and the to read bit length only 1.
	// Positive values must be subtracted with 31 to produce the final result.
	// The lookup procedure starts with the reading of 3 bits. These values are the indices for column 0.
	private final static byte HUFFMANCODETABLE7_23[][] = {
		{31, 32, 30, -1, -2, -3, -4, -5},
		{33, 29},
		{34, 28},
		{35, 27, 36, 26, 37, 25, 38, 24},
		{39, 23, 40, 22, 41, 21, 42, 20, 43, 19, 44, 18, 45, 17, 46, 16},
		{47, 15, 48, 14, 49, 13, 50, 12, 51, 11, 52, 10, 53,  9, 54,  8, 55,  7, 56,  6, 57,  5, 58,  4, 59,  3, 60,  2, 61,  1, 62,  0},
	};
	private final static byte TOKEN_TABLE7_33[][] = {
		{0, 0, 0, 2, 3,  4, 12, 3, 6, 0,  0, 0,  0, 1, 1, 1, 1, 2, 3,  4,  5,  6, 10, 1, 1, 1, 1, 1, 3,  4,  2, 3},
		{0, 0, 0, 3, 7, 15, -1, 0, 0, 1, -1, 2, -2, 3, 4, 5, 6, 7, 9, 13, 21, 37, 69, 1, 2, 3, 4, 5, 6, 10,  0, 1},
		{0, 0, 0, 0, 0, 0,   0, 1, 1, 2,  2, 2,  2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2, 3, 3, 3, 3, 3, 3,  3,  4, 4}
	};
	private final static byte HUFFMAN_GROUP_TABLE7_42[] = {
		0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,
		3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
		4, 4, 4, 4
	};
	private final static byte CODE_MODE_TABLE7_46[] = {
		1, 0, 1, 1, 1, 2, 2, 1
	};
	private final static short DCPREDICTORS_WEIGHTS_AND_DIVISORS_TABLE7_47[][] = {
		{ 0,   0,   0,   0,   0},
		{ 1,   0,   0,   0,   1},
		{ 1,   0,   0,   0,   1},
		{ 1,   0,   0,   0,   1},
		{ 1,   0,   0,   0,   1},
		{ 1,   1,   0,   0,   2},
		{ 0,   1,   0,   0,   1},
		{29, -26,  29,   0,  32},
		{ 1,   0,   0,   0,   1},
		{75,  53,   0,   0, 128},
		{ 1,   1,   0,   0,   2},
		{75,   0,  53,   0, 128},
		{ 1,   0,   0,   0,   1},
		{75,   0,  53,   0, 128},
		{ 3,  10,   3,   0,  16},
		{29, -26,  29,   0,  32}
	};
	// [yRemainder][xRemainder][bi] // index plus/minus one to avoid -0 / +0!
	// Note, that Hilbert curve pattern was choosen,
	// because the fractal dimension of a Hilbert curve aproximates to 2.
	private final static byte CHROMA_420_BI_TO_MBI_MAPPING[][][] = {
	   {{ 1            },
	    { 1,  2        },
	    { 1,  2,  3    },
	    { 1,  2,  3,  4}},
	   {{ 1,  2,       },
	    { 1,  4,  3,  2},
	    { 1,  4,  3,  2,
	      6,  5        },
	    { 1,  4,  3,  2,
	      7,  6,  5,  8}},
	   {{ 1,  2, -1    },
	    { 1,  4,  3,  2,
	     -1, -2       },
	    { 1,  4,  3,  2,
	     -1, -2, -3,
	      6,  5        },
	    { 1,  4,  3,  2,
	     -1, -2,
	     -3, -4,
	      7,  6,  5,  8}},
	   {{ 1,  2, -1, -2},
	    { 1,  4,  3,  2,
	     -1, -2, -3, -4},
	    { 1,  4,  3,  2,
	     -1, -2, -3, -4,
	     -5, -6,  6,  5},
	    { 1,  4,  3,  2,
	     -1, -2, -3, -4,
	     -5, -6, -7, -8,
	      7,  6,  5,  8}}
	};
	// [yRemainder / 2][xRemainder][bi] // index plus/minus one to avoid -0 / +0!
	// Note, that Hilbert curve pattern was choosen,
	// because the fractal dimension of a Hilbert curve aproximates to 2.
	private final static byte CHROMA_422_BI_TO_MBI_MAPPING[][][] = {
	   {{ 1,  1        },
	    { 1,  2,  2,  1},
	    { 1,  2,  2,  1,
	      3,  3,       },
	    { 1,  2,  2,  1,
	      4,  3,  3,  4}},
	   {{ 1,  1,  2,  2},
	    { 1,  4,  4,  1,
	      2,  2,  3,  3},
	    { 1,  4,  4,  1,
	      2,  2,  3,  3,
	      6,  6,  5,  5},
	    { 1,  4,  4,  1,
	      2,  2,  3,  3,
	      6,  6,  7,  7,
	      8,  5,  5,  8}}
	};
	// The first index gets 1 if the superblock has 4 macro blocks or is truncated
	// along the x-axis or 0 if the the superblock is truncated along the y-axis,
	// the second index gets the luma macro block indices (lmbi) and the third index
	// gets the block indices (coding order) and the result are raster ordered block indices.
	private final static byte CODING_ORDER_TO_BLOCK_RASTER_ORDER[][][] = {
	   {{ 0,  1,  3,  2},
	    { 2,  3,  1,  0}},
	   {{ 0,  1,  3,  2},
	    { 0,  3,  1,  2},
	    { 0,  3,  1,  2},
	    { 2,  3,  1,  0}}
	};
	// The first index gets 1 if the superblock has 4 macro blocks or is truncated
	// along the x-axis or 0 if the the superblock is truncated along the y-axis,
	// the second index gets the block indices (coding order).
	private final static byte CHROMA_422_MOTIONVECTOR_SELECTION[][] = {
	    { 4,  4,  5,  5,
	      5,  5,  4,  4},
	    { 4,  4,  5,  5,
	      4,  5,  5,  4,
	      4,  5,  5,  4,
	      5,  5,  4,  4}
	};
	// [chromaDecimation][mva + 31]
	private final static byte MOTIONVECTOR_MAPPING_BASE[][] = {
	   {-15, -15, -14, -14, -13, -13, -12, -12, -11, -11, -10, -10,  -9,  -9,  -8,
	     -8,  -7,  -7,  -6,  -6,  -5,  -5,  -4,  -4,  -3,  -3,  -2,  -2,  -1,  -1,   0,
	      0,   0,   1,   1,   2,   2,   3,   3,   4,   4,   5,   5,   6,   6,   7,   7,
	      8,   8,   9,   9,  10,  10,  11,  11,  12,  12,  13,  13,  14,  14,  15,  15},
	   { -7,  -7,  -7,  -7,  -6,  -6,  -6,  -6,  -5,  -5,  -5,  -5,  -4,  -4,  -4,
	     -4,  -3,  -3,  -3,  -3,  -2,  -2,  -2,  -2,  -1,  -1,  -1,  -1,   0,   0,   0,
	      0,   0,   0,   0,   1,   1,   1,   1,   2,   2,   2,   2,   3,   3,   3,   3,
 	      4,   4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   7}
	};
	// [chromaDecimation][mva + 31]
	private final static byte MOTIONVECTOR_MAPPING_DIFF[][] = {
	   { -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,
 	      0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,   0,  -1,
 	      0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,
 	      0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1},
	   { -1,  -1,  -1,   0,  -1,  -1,  -1,   0,  -1,  -1,  -1,   0,  -1,  -1,  -1,
 	      0,  -1,  -1,  -1,   0,  -1,  -1,  -1,   0,  -1,  -1,  -1,   0,  -1,  -1,  -1,
 	      0,   1,   1,   1,   0,   1,   1,   1,   0,   1,   1,   1,   0,   1,   1,   1,
 	      0,   1,   1,   1,   0,   1,   1,   1,   0,   1,   1,   1,   0,   1,   1,   1}
	};
	private final static short[] ONE_HUNDRED_TWENTYEIGHT_INTS = {
		128, 128, 128, 128, 128, 128, 128, 128
	};
	private short[][][][] qmat; // A 64-element array of quantization values for each DCT coefficient in natural order.
	private boolean initialized;
	private short[][] image; // An array containing the contents of all planes of the current frame.
	private short[][] ref; // An array containing the contents of all planes of the reference frame (past frame).
	private short[][] goldRef; // An array containing the contents of all planes of the golden reference frame (past key frame).
	private byte[] mAlphabet = new byte[8]; // Integer array - The list of modes corresponding to each Huffman code.
	private int nbs; // The number of blocks. // Porting problem: Should be long
	private int nsbs; // The number of super blocks.
	private int nlsbs; // The number of luma plane super blocks.
	private int ncsbs; // The number of chroma plane super blocks.
	private int nmbs; // The number of macro blocks.
	private int fType; // The frame type.
	private int nqis; // The number of qi values.
	private int[] qis = new int[3]; // A NQIS-element array of qi values.
	private byte[] tis; // A NSBS*NBSPS-element array of the current token index for each block.
	private byte[][] bCoded; // A NSBS/NBSPS array indicating which blocks are coded.
	private int[] cbIndex; // A NBS-element array of flags (sbi | bi) coding coded block indices. // Porting problem: Should be long[]
	private Node qcbIndex;
	private Node root;
	private int[] ncbIndex; // A NBS-element array of flags (sbi | bi) coding not coded intra frame block indices. // Porting problem: Should be long[]
	private int[] cifbIndex; // A NBS-element array of flags (sbi | bi) coding coded intra frame block indices. // Porting problem: Should be long[]
	private int[] ncifbIndex; // A NBS-element array of flags (sbi | bi) coding not coded block indices. // Porting problem: Should be long[]
	private int[] mbiToSbiLmbiMapping; // A NMBS-element array of flags (sbi | lmbi | ySbRemainder / 2) coding macro block indices.
	private int[] biToMbiMapping; // A NSBS*NBSPS array of macro block indices.
	private int[][][] rasterOrderToCodedOrderMapping; // A NPLS/NBY/NBX array of flags (sbi | bi).
	private byte[] mbMode; // A NMBS-element array of coding modes for each macro block.
	private byte[][][] mVects; // A NMBS/NBSPMB/MVECXY array of motion vectors.
	private byte[] qiis; // A NBS-element array of qii values for each block.
	private int ncbs; // Number of coded blocks. // Porting problem: Should be long
	private int nncbs; // Number of not coded blocks. // Porting problem: Should be long
	private short[][] coeffs; // A NSBS*NBSPS/64 array of quantized DCT coefficient values for each block in zig-zag order.
	private byte[] nCoeffs; // A NSBS*NBSPS-element array of the coefficient count for each block.
	private short[] lastDc = new short[3]; // A 3-element array containing the most recently decoded DC value, one for inter mode and for each reference frame.
	private short[] coeffsValues = new short[4]; // A 4-element array of the neighboring DC coefficient values to apply to each DC value.
	private int[] spipp; // A NSBS-element array of the spi's of each single plane.
	private int[] pli; // A NSBS-element array of the plane index values.
	private int y0, y1, y2, y3, y4, y5, y6, y7; // An 8-element array of 1D iDCT input values.
	private int[][] planeWidthClip; // An array for clipping/clamping plane sizes for each plane. The third plane clip array is a reused second one.
	private int[][] planeHeightClip; // An array for clipping/clamping plane sizes for each plane. The third plane clip array is a reused second one.
	private byte[] lflims; // A 64-element array of loop filter limit values.
	private byte[] sbpCodedBits; // Bit string of a decoded set of partial coded super block flags. // Porting problem: Should be long
	private byte[] sbfCodedBits; // Bit string of a decoded set of full coded super block flags. // Porting problem: Should be long
	private byte[] bits; // Bit string of a decoded set of flags. // Porting problem: Should be long
	private int eobs; // The remaining length of the current EOB run. // Porting problem: Should be long
	private int[] blockCoordinateX; // A NSBS*NBSPS-element array of the horizontal pixel indices of the lower-left corner of the current block.
	private int[] blockCoordinateY; // A NSBS*NBSPS-element array of the vertical pixel indices of the lower-left corner of the current block.
	private short[][] lflim = new short[64][FILTER_RANGE];
	private int[] dqc = new int[64]; // An array of dequantized DCT coefficients in natural order.
	private boolean[] isNb = new boolean[4]; // Is there a neighbor block for inverting the DC Prediction Process?
	private int sbChromaWidth;
	private int sbChromaHeight;
	private int sbLumaWidth;
	private int sbLumaHeight;
	private int bChromaWidth;
	private int bChromaHeight;
	private int bLumaWidth;
	private int bLumaHeight;
	private int mbWidth;
	private int mbHeight;
	private byte[] zeroBytes;
	private byte[] oneBytes;
	private byte[][] oneBytesBytes;
	private int pixelOffset;
	private int pixelRange;
	private int maximumPixelSize;
	private int[] xBLength = new int[3];
	private int[] yBLength = new int[3];
	private int[] yBStart = new int[3];
	private int[] yBEnd = new int[3];
	private int externFrameCount;
	private int internFrameCount;
	private boolean restart;
	
	static {
		int i;
		
		for(i = -CLIP_MIN; i < CLIP_MAX; i++) {
			clip[i + CLIP_MIN] = (short) (i >= 0 ? i <= 255 ? i : 0xFF : 0);
		}
	}
		
	/**
	 * Constructs an instance of <code>TheoraDecoder</code> with a
	 * <code>VideoReader</code> object containing all necessary informations
	 * about the media source.
	 *
	 * @param info     the <code>VideoReader</code> object containing all
	 *      necessary informations about the media source
	 */
	TheoraDecoder(VideoReader info) {
		super(info);
		
		nbs = info.blocks;
		nsbs = info.superblocks;
		nmbs = info.macroblocks;
		sbChromaWidth = info.sbChromaWidth;
		sbChromaHeight = info.sbChromaHeight;
		sbLumaWidth = info.sbLumaWidth;
		sbLumaHeight = info.sbLumaHeight;
		mbWidth = info.mbWidth;
		mbHeight = info.mbHeight;
		qmat = info.qmat;
		lflims = info.lflims;
		maximumPixelSize = codedPictureWidth * codedPictureHeight;

		chromaWidth = chromaFormat == CHROMA444 ? codedPictureWidth : codedPictureWidth >>> 1;
		chromaHeight = chromaFormat != CHROMA420 ? codedPictureHeight : codedPictureHeight >>> 1;
		
		bChromaWidth = chromaWidth >>> 3;
		bChromaHeight = chromaHeight >>> 3;
		bLumaWidth = codedPictureWidth >>> 3;
		bLumaHeight = codedPictureHeight >>> 3;
		
		nlsbs = sbLumaHeight * sbLumaWidth;
		ncsbs = sbChromaHeight * sbChromaWidth;
		
		if (image == null) {
			image = new short[3][];
			image[0] = new short[maximumPixelSize];
			image[1] = new short[chromaWidth * chromaHeight];
			image[2] = new short[chromaWidth * chromaHeight];
			ref  = new short[3][];
			ref[0] = new short[maximumPixelSize];
			ref[1] = new short[chromaWidth * chromaHeight];
			ref[2] = new short[chromaWidth * chromaHeight];
			goldRef = new short[3][];
			goldRef[0] = new short[maximumPixelSize];
			goldRef[1] = new short[chromaWidth * chromaHeight];
			goldRef[2] = new short[chromaWidth * chromaHeight];
		}
		
		zeroBytes = new byte[nsbs << 4];
		oneBytes = new byte[nbs];
		bCoded = new byte[nsbs][]; // Porting problem: Index can't be of type long (32 bit unsigned)
	  	oneBytesBytes = new byte[nsbs][]; // Porting problem: Index can't be of type long (32 bit unsigned)
	  	cbIndex = new int[nbs]; // Porting problem: Index can't be of type long (36 bit unsigned)
		root = qcbIndex = new Node();
		ncbIndex = new int[nbs]; // Porting problem: Index can't be of type long (36 bit unsigned)
		cifbIndex = new int[nbs]; // Porting problem: Index can't be of type long (36 bit unsigned)
		qiis = new byte[nsbs << 4]; // Porting problem: Index can't be of type long (36 bit unsigned)
		tis = new byte[nsbs << 4]; // Porting problem: Index can't be of type long (36 bit unsigned)
		coeffs = new short[nsbs << 4][64];
		nCoeffs = new byte[nsbs << 4];
		blockCoordinateX = new int[nsbs << 4];
		blockCoordinateY = new int[nsbs << 4];
		sbpCodedBits = new byte[nsbs]; // Porting problem: Should be long
		sbfCodedBits = new byte[nsbs]; // Porting problem: Should be long
		bits = new byte[nbs]; // Porting problem: Should be long

		int blocksPerMakroblock = 4; // index 4 until 8 and 8 until 12 are equal to index 0 until 3
		
		if (chromaFormat == CHROMA420) {
			blocksPerMakroblock = 5; // index 4 and 5 are equal
		} else if (chromaFormat == CHROMA422) {
			blocksPerMakroblock = 6; // index 4 and 5 are equal to index 6 and 7
		}

		mbMode = new byte[nmbs];
		mbiToSbiLmbiMapping = new int[nmbs];
		mVects = new byte[nmbs][blocksPerMakroblock][2];
		biToMbiMapping = new int[nsbs << 4];
		
		int cpli, sbx, sby, sbi = 0, bi, sbibi, xSbEnd, ySbEnd, xBEnd, yBEnd, xEnd, yEnd;
		int xSbLength[] = {sbLumaWidth, sbChromaWidth, sbChromaWidth};
		int ySbLength[] = {sbLumaHeight, sbChromaHeight, sbChromaHeight};
		
		xBLength[0] = bLumaWidth;
		xBLength[1] = xBLength[2] = bChromaWidth;
		yBLength[0] = bLumaHeight;
		yBLength[1] = yBLength[2] = bChromaHeight;
		
		int xSbRemainder;
		int ySbRemainder;
		int size;
		int mbi;
		int oneBytesIndex = 0;
		
		spipp = new int[nsbs];
		pli = new int[nsbs];
		planeWidthClip = new int[3][];
		planeHeightClip = new int[3][];
		
		int[] planeWidthClipPointer = null;
		int[] planeHeightClipPointer = null;
		
		int bx, by;

		int mbWidth2 = mbWidth << 1;
		int mbWidth4 = mbWidth << 2;
		int ySbRemainderMinusOne;
		int xSbRemainderMinusOne;
		int ySbRemainderHalfMinusOne;
		int ySbRemainder2;
		int biShiftRightTwo;
		
		rasterOrderToCodedOrderMapping = new int[3][][];
		
		int[][] rasterOrderToCodedOrderMappingPointer;
		
		for (cpli = 0; cpli < 3; cpli++) {
			xSbEnd = xSbLength[cpli];
			ySbEnd = ySbLength[cpli];
			xBEnd = xBLength[cpli];
			yBEnd = yBLength[cpli];
			xEnd = xBEnd << 3;
			yEnd = yBEnd << 3;
			rasterOrderToCodedOrderMappingPointer = rasterOrderToCodedOrderMapping[cpli] = new int[yBEnd][xBEnd];
			if (cpli < 2) {
				planeWidthClipPointer = planeWidthClip[cpli] = new int[xEnd + PLANE_CLIP_RANGE];
				planeHeightClipPointer = planeHeightClip[cpli] = new int[yEnd + PLANE_CLIP_RANGE];
				for (sbx = -PLANE_CLIP_MIN; sbx < xEnd + PLANE_CLIP_MAX; sbx++) {
					planeWidthClipPointer[sbx + PLANE_CLIP_MIN] = sbx >= 0 ? sbx < xEnd ? sbx : xEnd - 1 : 0;
				}
				for (sby = -PLANE_CLIP_MIN; sby < yEnd + PLANE_CLIP_MAX; sby++) {
					planeHeightClipPointer[sby + PLANE_CLIP_MIN] = sby >= 0 ? sby < yEnd ? sby : yEnd - 1 : 0;
				}
			} else {
				planeWidthClip[2] = planeWidthClipPointer;
				planeHeightClip[2] = planeHeightClipPointer;
			}

			for (sby = 0; sby < ySbEnd; sby++) {
				if (sby == ySbEnd - 1) {
					ySbRemainder = yBEnd & 0x3;
					if (ySbRemainder == 0) {
						ySbRemainder = 4;
					}
				} else {
					ySbRemainder = 4;
				}
				ySbRemainderMinusOne = ySbRemainder - 1;
				ySbRemainderHalfMinusOne = ySbRemainder / 2 - 1;
				ySbRemainder2 = ySbRemainder << 1;
				
				for (sbx = 0; sbx < xSbEnd; sbx++) {
					pli[sbi] = cpli;
					if (sbx == xSbEnd - 1) {
						xSbRemainder = xBEnd & 0x3;
						if (xSbRemainder == 0) {
							xSbRemainder = 4;
						}
					} else {
						xSbRemainder = 4;
					}
					xSbRemainderMinusOne = xSbRemainder - 1;
					size = xSbRemainder * ySbRemainder;

					byte[] oneBytesBytesPointer = oneBytesBytes[sbi] = new byte[size];
					
					bCoded[sbi] = new byte[size];
					
					for (bi = 0; bi < size; bi++) {
						oneBytesBytesPointer[bi] = 1;
						oneBytes[oneBytesIndex++] = 1;
						biShiftRightTwo = bi >>> 2;
						sbibi = sbi << 4 | bi;
						if (cpli == 0 || chromaFormat == CHROMA444) {
							mbi = biToMbiMapping[sbibi] = biShiftRightTwo + sbx * ySbRemainder + sby * mbWidth2;
							if (cpli == 0) {
								mbiToSbiLmbiMapping[mbi] = sbi << 3 | biShiftRightTwo << 1 | ySbRemainder >>> 2;
							}
						} else {
							if (chromaFormat == CHROMA422) {
								biToMbiMapping[sbibi] = CHROMA_422_BI_TO_MBI_MAPPING[ySbRemainderHalfMinusOne][xSbRemainderMinusOne][bi] + sbx * ySbRemainder2 + sby * mbWidth2 - 1;
							} else {
								int value = CHROMA_420_BI_TO_MBI_MAPPING[ySbRemainderMinusOne][xSbRemainderMinusOne][bi];
								
								ySbRemainder2 = 8;

								if (value < 0) {
									value = -value;
									if (ySbRemainder == 3) {
										ySbRemainder2 = 4;
									}
									value += sbx * ySbRemainder2 + mbWidth2 + sby * mbWidth4 - 1;
								} else {
									if (ySbRemainder == 1) {
										ySbRemainder2 = 4;
									}
									value += sbx * ySbRemainder2 + sby * mbWidth4 - 1;
								}
								biToMbiMapping[sbibi] = value;
		 					}
						}
						cifbIndex[ncbs++] = sbibi;
						qcbIndex = generate(sbibi, qcbIndex);
						bx = sbx * 4 + SUPERBLOCK_X_BLOCKS_OFFSET[ySbRemainderMinusOne][xSbRemainderMinusOne][bi];
						by = sby * 4 + SUPERBLOCK_Y_BLOCKS_OFFSET[ySbRemainderMinusOne][xSbRemainderMinusOne][bi];
						rasterOrderToCodedOrderMappingPointer[by][bx] = sbibi;
						blockCoordinateX[sbibi] = bx << 3;
						blockCoordinateY[sbibi] = by << 3;
					}
					
					if (cpli == 0) {
						spipp[sbi] = sbi;
					} else if (cpli == 1) {
						spipp[sbi] = sbi - nlsbs;
					} else {
						spipp[sbi] = sbi - nlsbs - ncsbs;
					}
					sbi++;
				}
			}
		}
		
		int qi; // The quantization index.
		short r; // The edge detector response values.
		short l; // The loop filter limit value.
		short[] lflimPointer;

		for (qi = 0; qi < 64; qi++) {
			lflimPointer = lflim[qi];
			l = lflims[qi];
			for (r = 0; r < l; r++) {
				lflimPointer[-r - l + FILTER_MIN] = (short) ( r - l);
			 	lflimPointer[-r + FILTER_MIN] = (short) -r;
			 	lflimPointer[ r + FILTER_MIN] = (short)  r;
				lflimPointer[ r + l + FILTER_MIN] = (short) (-r + l);
			}
		}
	}

	/**
	 * Decodes an still image.
	 *
	 * @exception IOException  if an I/O error occurs
	 * @exception InterruptedIOException  if the decoding process is interrupted
	 *      caused from malformed media data
	 */
	public synchronized void decodeStillImage() throws IOException {
		skippedFrame = false;
		try {
			decode();
			restart = true;
		} catch (EndOfPacketException e) {
			skippedFrame = true;
		}
	}

	/**
	 * Decodes an image.
	 *
	 * @exception IOException  if an I/O error occurs
	 * @exception InterruptedIOException  if the decoding process is interrupted
	 *      caused from malformed media data
	 */
	public synchronized void decodeImage() throws IOException {
		skippedFrame = false;
		try {
			restart = false;
			decode();
		} catch (EndOfPacketException e) {
			skippedFrame = true;
		}
	}
	
	void decode() throws IOException, EndOfPacketException {
		
		pixelOffset = maximumPixelSize;
		
		int pixelMax = -1;
		
		yBEnd[0] = yBEnd[1] = yBEnd[2] = 0;
		yBStart[0] = yBLength[0];
		yBStart[1] = yBStart[2] = yBLength[1];
				
		if (get1() != 0) {
			throw new InterruptedIOException("No video packet");
		}
		
		// 7.1 Frame Header Decode
		
		int moreQis; // A flag indicating there are more qi values to be decoded.
		
		int getBuffer = get(8);
		
		fType = getBuffer >>> 7;
		qis[0] = getBuffer >>> 1 & 0x3F;
		moreQis = getBuffer & 0x1;
		
		if (moreQis == 0) {
			nqis = 1;
		} else {
			getBuffer = get(7);
			
			qis[1] = getBuffer >>> 1;
			moreQis = getBuffer & 0x1;
			if (moreQis == 0) {
				nqis = 2;
			} else {
				qis[2] = get(6);
				nqis = 3;
			}
		}
		if (fType == INTRA_FRAME) {
			pixelOffset = 0;
			pixelRange = maximumPixelSize;
			yBStart[0] = yBStart[1] = yBStart[2] = 0;
			yBEnd[0] = yBLength[0];
			yBEnd[1] = yBEnd[2] = yBLength[1];
			if (get(3) != 0) {
				throw new InterruptedIOException("Reserved values should be zero bytes");
			}
		}

		// 7.3 Coded Block Flags Decode
		// 7.4 Macro Block Coding Modes
		
		int nBits; // The length of a bit string to decode. // Porting problem: Should be long
		int sbi; // The index of the current super block.
		int bi; // The index of the current block in coded order.
		int mbi = 0; // The index of the current macro block.
		int lmbi = 0; // The index of the current luma macro block.
		int psbi = -1; // The index of the previous super block.
		int sbibi; // The index source of the index of the current super block and block.
		
		byte[] bCodedPointer;
		byte[][] bCodedBuffer = bCoded;
		byte[] mbModeBuffer = mbMode;
		int[] cbIndexBuffer = cbIndex;
		int bitCounter = 0; // Porting problem: Should be long
		int biShiftRightTwo = 0;
		byte mvx; // The horizontal component of the first whole-pixel motion vector.
		byte mvy; // The vertical component of the first whole-pixel motion vector.
		byte[][] mVectsPointer;
		byte[] mVectsPointerArray;
		byte mode;
		
		ncbs = 0;
		nncbs = 0;
		
		qcbIndex = root;

		if (fType == INTRA_FRAME) {
			mbMode = oneBytes;
			bCoded = oneBytesBytes;
			cbIndex = cifbIndex;
			ncbs = nbs;
			qcbIndex = rebuildIntraCbIndexQueue(nbs, cbIndex, qcbIndex);
		} else {
			byte sbpCodedBit;
			byte sbfCodedBit = 0;
			int sbfCodedBitCounter = 0; // Porting problem: Should be long
			int mScheme; // The mode coding scheme.
			boolean[] lumaMakroBlockCoded = new boolean[4];
			int lumaMakroBlockCodedValue;
			
			nBits = nsbs;
			
			runBitStringDecode(nBits, sbpCodedBits, HUFFMANCODETABLE7_7RSTART, HUFFMANCODETABLE7_7RBITS);
			
			for (sbi = nBits = 0; sbi < nsbs; sbi++) {
				if (sbpCodedBits[sbi] == 0) {
					nBits++;
				}
			}

			if (nBits > 0) {

				runBitStringDecode(nBits, sbfCodedBits, HUFFMANCODETABLE7_7RSTART, HUFFMANCODETABLE7_7RBITS);
			
				for (sbi = nBits = 0; sbi < nsbs; sbi++) {
					if (sbpCodedBits[sbi] != 0) {
						nBits += bCoded[sbi].length;
					}
				}
			}

			runBitStringDecode(nBits, bits, HUFFMANCODETABLE7_11RSTART, HUFFMANCODETABLE7_11RBITS);
			
			mScheme = get(3);
			
			if (mScheme == 0) {
				for (mode = 0; mode < 8; mode++) {
					// get(3) == The index of a Huffman code from Table 7.19, starting from 0
					mAlphabet[get(3)] = mode;
				}
			} else if (mScheme != 7) {
				System.arraycopy(HUFFMANCODETABLE7_14[mScheme], 0, mAlphabet, 0, mAlphabet.length);
			}
				
			for (sbi = 0; sbi < nsbs; sbi++) {

				bCodedPointer = bCoded[sbi];
			
				sbpCodedBit = sbpCodedBits[sbi];
				
				if (sbpCodedBit == 0) {
					sbfCodedBit = sbfCodedBits[sbfCodedBitCounter++];
				}
	
				lumaMakroBlockCoded[0] = false;
				lumaMakroBlockCoded[1] = false;
				lumaMakroBlockCoded[2] = false;
				lumaMakroBlockCoded[3] = false;

				for (bi = 0; bi < bCodedPointer.length; bi++) {

					biShiftRightTwo = bi >>> 2;

					lumaMakroBlockCodedValue = bCodedPointer[bi] = sbpCodedBit == 0 ? sbfCodedBit : bits[bitCounter++];
					
					if (lumaMakroBlockCodedValue != 0) {
						sbibi = sbi << 4 | bi;
						cbIndex[ncbs++] = sbibi;
						qcbIndex = put(sbibi, qcbIndex);
						lumaMakroBlockCoded[biShiftRightTwo] = true;
					} else {
						ncbIndex[nncbs++] = sbi << 4 | bi;
					}
				}
				if (sbi < nlsbs) {
					int lmbiEnd = biShiftRightTwo + 1;

					for (lmbi = 0; lmbi < lmbiEnd; lmbi++, mbi++) {
						mbMode[mbi] = lumaMakroBlockCoded[lmbi] ? mScheme != 7 ? mAlphabet[getHuffcodeTableIndex(mAlphabet.length)] : (byte) get(3) : INTER_NOMV;
					}
				}
			}
		
			// 7.5 Motion Vectors
			// 7.5.2 Macro Block Motion Vector Decode
		
			byte last1x = 0; // The last motion vector x value.
			byte last1y = 0; // The last motion vector y value.
			byte last2x = 0; // The last motion vector x value.
			byte last2y = 0; // The last motion vector y value.
			byte[] mVectsChroma1PointerArray;
			byte[] mVectsChroma2PointerArray;
			int mvMode = get1(); // The motion vector decoding method.
			int mbModePointerValue;
			int bri; // block index in raster order.
			byte[] orderMappingPointer;
			int sum, sumABx, sumABy, sumCDx, sumCDy;
			byte mbModeValue = 0;
			int mbiToSbiLmbiMappingValue;
			int lmbiShiftLeftTwo;
			
			bCodedPointer = null;
		
			if (ncbs > 0) {
				for (mbi = 0; mbi < mVects.length; mbi++) {
					mbiToSbiLmbiMappingValue = mbiToSbiLmbiMapping[mbi];
					sbi = mbiToSbiLmbiMappingValue >>> 3;
					lmbi = mbiToSbiLmbiMappingValue >>> 1 & 0x3;
					lmbiShiftLeftTwo = lmbi << 2;
					mbModeValue = mbMode[mbi];
					mVectsPointer = mVects[mbi];
					orderMappingPointer = CODING_ORDER_TO_BLOCK_RASTER_ORDER[mbiToSbiLmbiMappingValue & 0x1][lmbi];
					if (psbi != sbi) {
						bCodedPointer = bCoded[sbi];
						psbi = sbi;
					}
					mVectsPointerArray = mVectsPointer[0];
					mVectsPointerArray[0] = mVectsPointerArray[1] = 0;
			 		if (mbModeValue == INTER_MV_FOUR) {
						last2x = last1x;
						last2y = last1y;
						sumABx = sumABy = sumCDx = sumCDy = 0;
						for (bi = 0; bi < 4; bi++) {
							bri = orderMappingPointer[bi];
						
							mVectsPointerArray = mVectsPointer[bri];
						
							if (bCodedPointer[lmbiShiftLeftTwo | bri] != 0) {
								motionVectorDecode(mVectsPointerArray, mvMode);
			 					last1x = mVectsPointerArray[0];
								last1y = mVectsPointerArray[1];
								if (bri < 2) {
									sumABx += last1x;
									sumABy += last1y;
								} else {
									sumCDx += last1x;
									sumCDy += last1y;
								}
			 				} else {
								mVectsPointerArray[0] = mVectsPointerArray[1] = 0;
							}
						}
					
						mVectsChroma1PointerArray = mVectsPointer[4];
					
						if (chromaFormat == CHROMA420) {
							sum = sumABx + sumCDx;
							mVectsChroma1PointerArray[0] = (byte) ((sum + (sum < 0 ? -2 : 2)) / 4);

							sum = sumABy + sumCDy;
							mVectsChroma1PointerArray[1] = (byte) ((sum + (sum < 0 ? -2 : 2)) / 4);
						} else if (chromaFormat == CHROMA422) {
							mVectsChroma2PointerArray = mVectsPointer[5];
				
							mVectsChroma1PointerArray[0] = (byte) ((sumABx + (sumABx < 0 ? -1 : 1)) / 2);
							mVectsChroma2PointerArray[0] = (byte) ((sumCDx + (sumCDx < 0 ? -1 : 1)) / 2);
							mVectsChroma1PointerArray[1] = (byte) ((sumABy + (sumABy < 0 ? -1 : 1)) / 2);
							mVectsChroma2PointerArray[1] = (byte) ((sumCDy + (sumCDy < 0 ? -1 : 1)) / 2);
						}
					} else if (mbModeValue == INTER_GOLDEN_MV) {
						motionVectorDecode(mVectsPointerArray, mvMode);
					} else if (mbModeValue == INTER_MV_LAST2) {
						mVectsPointerArray[0] = last2x;
						mVectsPointerArray[1] = last2y;
						last2x = last1x;
						last2y = last1y;
						last1x = mVectsPointerArray[0];
						last1y = mVectsPointerArray[1];
					} else if (mbModeValue == INTER_MV_LAST) {
						mVectsPointerArray[0] = last1x;
						mVectsPointerArray[1] = last1y;
					} else if (mbModeValue == INTER_MV) {
						motionVectorDecode(mVectsPointerArray, mvMode);
						last2x = last1x;
						last2y = last1y;
						last1x = mVectsPointerArray[0];
						last1y = mVectsPointerArray[1];
					}
					if (mbModeValue != INTER_MV_FOUR) {
						mvx = mVectsPointerArray[0];
						mvy = mVectsPointerArray[1];
						for (bi = 1; bi < mVectsPointer.length; bi++) {
							mVectsPointerArray = mVectsPointer[bi];
							mVectsPointerArray[0] = mvx;
							mVectsPointerArray[1] = mvy;
						}
					}
				}
			}
		}
		
		// 7.6 Block-Level qi Decode
		
		if(ncbs > 0) {
			int qii; // The index of qi value in the list of qi values defined for this frame.
			int cbi; // The index of the current block in the coded block list.
			int qiiPlusOne;
		
			nBits = ncbs;
			System.arraycopy(zeroBytes, 0, qiis, 0, qiis.length);
		
			if (nqis > 1 && ncbs > 0) {
			
				runBitStringDecode(nBits, bits, HUFFMANCODETABLE7_7RSTART, HUFFMANCODETABLE7_7RBITS);
			
				for (qii = 0; ; qii++) {
					qiiPlusOne = qii + 1;
					for (cbi = bitCounter = nBits = 0; cbi < ncbs; cbi++) {
						sbibi = cbIndex[cbi];
						if (qiis[sbibi] == qii) {
							qiis[sbibi] += bits[bitCounter++];
							if (qiis[sbibi] == qiiPlusOne) {
								nBits++;
							}
						}
					}
					if (qii < nqis - 2) {
						runBitStringDecode(nBits, bits, HUFFMANCODETABLE7_7RSTART, HUFFMANCODETABLE7_7RBITS);
					} else {
						break;
					}
				}
			}
		
      		// 7.7 DCT Coefficients
			// 7.7.3 DCT Coefficient Decode
		
			int token; // No The current token being decoded.
			int hg; // No The current Huffman table group.
			int ti = 0; // The current token index.
			int htil = 0; // The index of the current Huffman table to use for the luma plane within a group.
			int htic = 0; // The  index of the current Huffman table to use for the chroma planes ithin a group.
			int hti; // The index of the current Huffman table to use.
			short[] coeffsPointer = null;
			int psbibi = -1;
		
			System.arraycopy(zeroBytes, 0, tis, 0, tis.length);
		
			eobs = 0;
		
			for (; ti < 64 ; ti++) {
				qcbIndex = root;
		
				if (ti == 0 || ti == 1) {
					getBuffer = get(8);
					htil = getBuffer >>> 4;
					htic = getBuffer & 0xF;
				}
				
				while (traversable(qcbIndex)) {
			
					qcbIndex = qcbIndex.next;
				
					sbibi = qcbIndex.index;
					
					if (psbibi != sbibi) {
						coeffsPointer = coeffs[sbibi];
						psbibi = sbibi;
					}
				
					if (tis[sbibi] == ti) {
						nCoeffs[sbibi] = (byte) ti;
					
						if (eobs != 0) {
							coeffsPointer[ti] = 0;
							tis[sbibi] = 64;
							remove(qcbIndex);
							eobs--;
						} else {
							hg = HUFFMAN_GROUP_TABLE7_42[ti];
							hti = pli[sbibi >>> 4] == 0 ? hg << 4 | htil : hg << 4 | htic;
						
							token = getCodeWord(hti);
						
							if (token < 7) {
								eobs = token < 3 ? token : get(TOKEN_TABLE7_33[0][token]) + TOKEN_TABLE7_33[1][token];
								coeffsPointer[ti] = 0;
								nCoeffs[sbibi] = tis[sbibi];
								tis[sbibi] = 64;
								remove(qcbIndex);
							} else {
								coefficientTokenDecode(coeffsPointer, nCoeffs, tis, token, sbibi, ti);
							}
						}
					}
				}
			}
		}
		
		// 7.8 Undoing DC Prediction
		// 7.8.1 Computing the DC Predictor
		// 7.8.2 Inverting the DC Prediction Process
		// 7.9 Reconstruction
		
		int ci; // The DCT coefficient index in natural order
				// in opposite to the DCT coefficient index in zig-zag order
				// == {int zzi = ZIG_ZAG_SCAN[ci];}.
		int dc = 0; // The actual DC value for the current block.
		int dcPred = 0; // The predicted DC value for the current block.
		int rfi; // The index of the reference frame indicated by the coding mode for macro block mbi.
		int cpli = 0; // The current plane index.
		int nbii; // The index of the index of neighbor blocks.
		int nbiiCounter; // The index of the index of neighbor blocks counter.
		int bj; // The index of a neighboring block in oded order.
		int sbj; // The index of a neighboring superblock in oded order.
		//int pdiv; // The valud to divide the weighted sum by.
		short[] weightsDivisorTableOuterPointer;
		int table47index;
		//byte mvx; // The horizontal component of the first whole-pixel motion vector.
		//byte mvy; // The vertical component of the first whole-pixel motion vector.
		byte mvx2 = mvx = 0; // The horizontal component of the second whole-pixel motion vector.
		byte mvy2 = mvy = 0; // The vertical component of the second whole-pixel motion vector.
		int qti; // A quantization type index. See Table 3.1.
		int qi0 = qis[0]; // The quantization index of the DC coefficient.
		short[] refp = null; // A RPH/RPW array containing the contents of the current plane of the reference frame.
		int refpi; // The pixel index of the reference plane pointer.
		short[] imp = null; // A RPH/RPW array containing the contents of the current plane of the current frame.
		int impi; // The pixel index of the current image plane pointer.
		int[] clipW = null;
		int[] clipH = null;
		int[] rasterOrderToCodedOrderMappingInnerPointer;
		int[][] rasterOrderToCodedOrderMappingOuterPointer;
		int pbx; // The horizontal pixel index of the lower left edge of the current block.
		int pby; // The vertical pixel index of the lower left edge of the current block.
		int bx; // The horizontal block index of the current block.
		int by; // The vertical block index of the current block.
		int nbx; // The horizontal neighbor block index of the current block.
		int nby; // The vertical neighbor block index of the current block.
		int wcps = 0; // The width of the current plane in pixels.
		int hcps = 0; // The height of the current plane in pixels.
		int wcbs = 0; // The width of the current plane in blocks.
		int hcbs; // The height of the current plane in blocks.
		int sbjbj; // The index source of the index of the neighbor super block and block.
		boolean sparseDc;
		byte[] chromaSelectionPointer;
		int nc;
		int pbyShift = 0;
		
		for (; cpli < 3 && ncbs > 0; cpli++) {
			System.arraycopy(ZERO_SHORTS, 0, lastDc, 0, 3);
			clipW = planeWidthClip[cpli];
			clipH = planeHeightClip[cpli];
			wcps = clipW.length - PLANE_CLIP_RANGE;
			hcps = clipH.length - PLANE_CLIP_RANGE;
			wcbs = wcps >>> 3;
			hcbs = hcps >>> 3;
			imp = image[cpli];
			rasterOrderToCodedOrderMappingOuterPointer = rasterOrderToCodedOrderMapping[cpli];
			if (cpli != 0 && chromaFormat == CHROMA420) {
				pbyShift = 1;
			}
			for (by = 0; by < hcbs; by++) {
				pby = by << 3;
				rasterOrderToCodedOrderMappingInnerPointer = rasterOrderToCodedOrderMappingOuterPointer[by];
				chromaSelectionPointer = CHROMA_422_MOTIONVECTOR_SELECTION[by == hcbs - 1 && hcbs % 4 != 0 ? 0 : 1];

				for (bx = 0; bx < wcbs; bx++) {
					pbx = bx << 3;
						
					sbibi = rasterOrderToCodedOrderMappingInnerPointer[bx];
					
					sbi = sbibi >>> 4;
					bi = sbibi & 0xF;
					
					mbi = biToMbiMapping[sbibi];

					if (bCoded[sbi][bi] != 0) {
						
						mode = mbMode[mbi];

						isNb[0] = bx != 0 ? true : false;
						isNb[1] = bx != 0 && by != 0 ? true : false;
						isNb[2] = by != 0 ? true : false;
						isNb[3] = bx != wcbs - 1 && by != 0 ? true : false;
					
						rfi = CODE_MODE_TABLE7_46[mode];
						
						for (nbii = table47index = nbiiCounter = 0; nbii < 4; nbii++) {
							if (isNb[nbii]) {
								nbx = nbii == 0 ? bx - 1 : bx + nbii - 2;
								nby = nbii == 0 ? by : by - 1;
								sbjbj = rasterOrderToCodedOrderMappingOuterPointer[nby][nbx];
					
								sbj = sbjbj >>> 4;
								bj = sbjbj & 0xF;
							
								if (bCoded[sbj][bj] != 0 && rfi == CODE_MODE_TABLE7_46[mbMode[biToMbiMapping[sbjbj]]]) {
									table47index |= 1 << nbii;
									coeffsValues[nbiiCounter++] = coeffs[sbjbj][0];
								}
							}
						}
						if (table47index == 0) {
							coeffs[sbibi][0] += lastDc[rfi];
						} else {
							weightsDivisorTableOuterPointer = DCPREDICTORS_WEIGHTS_AND_DIVISORS_TABLE7_47[table47index];
							dcPred = weightsDivisorTableOuterPointer[0] * coeffsValues[0];
							if (nbiiCounter > 1) {
								for (nbii = 1; nbii < nbiiCounter; nbii++) {
									dcPred += weightsDivisorTableOuterPointer[nbii] * coeffsValues[nbii];
								}
								dcPred /= weightsDivisorTableOuterPointer[4];
								if ((table47index & 0x7) == 7) {
									if (dcPred - coeffsValues[2] > 128 || dcPred - coeffsValues[2] < -128) {
										dcPred = coeffsValues[2];
									} else if (dcPred - coeffsValues[0] > 128 || dcPred - coeffsValues[0] < -128) {
										dcPred = coeffsValues[0];
									} else if (dcPred - coeffsValues[1] > 128 || dcPred - coeffsValues[1] < -128) {
										dcPred = coeffsValues[1];
									}
								}
							}
							coeffs[sbibi][0] += dcPred;
						}
						lastDc[rfi] = coeffs[sbibi][0];
					
						// 7.9.4 The Complete Reconstruction Algorithm
					
						qti = mode == INTRA ? 0 : 1;
						
						if (rfi != 0) {// Table 7.75: Reference Planes and Sizes for Each rfi and pli
							if (rfi == 1) {
								refp = ref[cpli];
							} else {
								refp = goldRef[cpli];
							}
							if (mode != INTER_NOMV && mode != INTER_GOLDEN_NOMV) {
								mVectsPointer = mVects[mbi];
				
								if (cpli == 0 || chromaFormat == CHROMA444) {
									mVectsPointerArray = mVectsPointer[bi & 0x3];
								} else if (chromaFormat == CHROMA422) {
									mVectsPointerArray = mVectsPointer[chromaSelectionPointer[bi]];
								} else {
									mVectsPointerArray = mVectsPointer[4];
								}
				
								int xChromaDecimation = (chromaFormat & 1) == 0 && cpli != 0 ? 1 : 0;

								int yChromaDecimation = (chromaFormat & 2) == 0 && cpli != 0 ? 1 : 0;
				
								mvx = MOTIONVECTOR_MAPPING_BASE[xChromaDecimation][mVectsPointerArray[0] + 31];
								mvy = MOTIONVECTOR_MAPPING_BASE[yChromaDecimation][mVectsPointerArray[1] + 31];
								mvx2 = MOTIONVECTOR_MAPPING_DIFF[xChromaDecimation][mVectsPointerArray[0] + 31];
								mvy2 = MOTIONVECTOR_MAPPING_DIFF[yChromaDecimation][mVectsPointerArray[1] + 31];
							} else {
								mvx = 0;
								mvy = 0;
								mvx2 = 0;
								mvy2 = 0;
							}
						}
			
						short[] qm0 = qmat[qti][cpli][qi0];
						short[] qm = qmat[qti][cpli][qis[qiis[sbibi]]];

						nc = nCoeffs[sbibi];
				
						sparseDc = false;

						if (nc < 2) {
							dc = (short) (coeffs[sbibi][0] * qm0[0] + 15 >> 5);
							if (dc == 0) {
								sparseDc = true;
							} else {
								for (ci = 0; ci < 64; ci++) {
									dqc[ci] = dc;
								}
							}
						} else {
							dequantization(qm0, qm, coeffs[sbibi], nc);
							if (nc <= 3) {
								inverseDCT2D3();
							} else if (nc <= 6) {
								inverseDCT2D6();
							} else if (nc <= 10) {
								inverseDCT2D10();
							} else {
								inverseDCT2D(ZIG_ZAG_TO_NATURAL_ORDER_ROW_MAPPING[nc - 1]);
							}
						}
						if (rfi == 0) {
							intraPredictor(imp, clipW, pbx, pby, sparseDc);
						} else {
							if (mvx2 != 0 || mvy2 != 0) {
								mvx2 += mvx;
								mvy2 += mvy;
								halfPixelPredictor(imp, refp, clipW, clipH, mvx, mvy, mvx2, mvy2, pbx, pby, sparseDc);
							} else {
								if (mvx != 0 || mvy != 0) {
									wholePixelPredictor(imp, refp, clipW, clipH, mvx, mvy, pbx, pby, sparseDc);
								} else {
									wholePixelPredictor(imp, refp, clipW, clipH, pbx, pby, sparseDc);
								}
							}
						}
						impi = (pby << pbyShift) * codedPictureWidth;
						
						if (impi > pixelMax) {
							pixelMax = impi;
							pixelRange = pixelMax + 8 * codedPictureWidth;
						}
						if (impi < pixelOffset) {
							pixelOffset = impi;
							pixelRange = pixelMax + 8 * codedPictureWidth;
						}
					}
					int byPlusOne = by + 1;
					
					if (byPlusOne > yBEnd[cpli]) {
						yBEnd[cpli] = byPlusOne;
					}
					if (by < yBStart[cpli]) {
						yBStart[cpli] = by;
					}
				}
			}
		}

		short[][] swap; // swap image pointer.
		int ppli = -1; // The last color plane index.
		int pbi = 0; // The index of the current pointed block in the non coded or coded block list.
		int pbs = ncbs; // An NBS-element array of pointed block indices // Porting problem: Should be long[]
		int[] pbIndex = cbIndex; // An NBS-element array of pointed block indices // Porting problem: Should be long[]

		if (ncbs > nncbs) {
			swap = image;
			image = ref;
			ref = swap;
			pbs = nncbs;
			pbIndex = ncbIndex;
		}

		for (psbi = -1; pbi < pbs; pbi++) {
			
			sbibi = pbIndex[pbi];
			
			sbi = sbibi >>> 4;
			
			if (psbi != sbi) {
				cpli = pli[sbi];
				if (ppli != cpli) {
					ppli = cpli;
					imp = image[cpli];
					refp = ref[cpli];
					clipW = planeWidthClip[cpli];
					clipH = planeHeightClip[cpli];
					wcps = clipW.length - PLANE_CLIP_RANGE;
					hcps = clipH.length - PLANE_CLIP_RANGE;
				}
				psbi = sbi;
			}
			
			impi = blockCoordinateY[sbibi] * wcps + blockCoordinateX[sbibi];
			System.arraycopy(imp, impi        , refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
			System.arraycopy(imp, impi += wcps, refp, impi, 8);
		}
		
		// 7.10 Loop Filtering
		// 7.10.3 Complete Loop Filter
		
		int fx; // The horizontal pixel index of the lower-left corner of the area to be filtered.
		int fy; // The vertical pixel index of the lower-left corner of the area to be filtered.
		int l = lflims[qi0]; // The loop filter limit value.
		
		if (l != 0) {
			short[] lflimPointer = lflim[qi0];
			int[] rasterOrderToCodedOrderMappingYPlusOne = null;
			int bxPlusOne;
			int byPlusOne;

			for (cpli = 0; cpli < 3; cpli++) {
				clipW = planeWidthClip[cpli];
				clipH = planeHeightClip[cpli];
				wcps = clipW.length - PLANE_CLIP_RANGE;
				hcps = clipH.length - PLANE_CLIP_RANGE;
				wcbs = wcps >>> 3;
				hcbs = hcps >>> 3;
				refp = ref[cpli];
				rasterOrderToCodedOrderMappingOuterPointer = rasterOrderToCodedOrderMapping[cpli];

				for (by = yBStart[cpli]; by < yBEnd[cpli]; by++) {
					pby = by << 3;

					rasterOrderToCodedOrderMappingInnerPointer = rasterOrderToCodedOrderMappingOuterPointer[by];
					byPlusOne = by + 1;

					if (byPlusOne < hcbs) {
						rasterOrderToCodedOrderMappingYPlusOne = rasterOrderToCodedOrderMappingOuterPointer[byPlusOne];
					}
					for (bx = 0; bx < wcbs; bx++) {
						pbx = bx << 3;

						sbibi = rasterOrderToCodedOrderMappingInnerPointer[bx];
					
						sbi = sbibi >>> 4;
						bi = sbibi & 0xF;

						if(bCoded[sbi][bi] == 0) {
							continue;
						}
					
						bxPlusOne = bx + 1;
						if (bx > 0) {
							fx = pbx - 2;
							fy = pby;
							horizontalFilter(refp, lflimPointer, fx, fy, wcps);
						}
						if (by > 0) {
							fx = pbx;
							fy = pby - 2;
							verticalFilter(refp, lflimPointer, fx, fy, wcps);
						}
						if (bxPlusOne < wcbs) {
							sbjbj = rasterOrderToCodedOrderMappingInnerPointer[bxPlusOne];
					
							sbj = sbjbj >>> 4;
							bj = sbjbj & 0xF;

							if(bCoded[sbj][bj] == 0) {
								fx = pbx + 6;
								fy = pby;
								horizontalFilter(refp, lflimPointer, fx, fy, wcps);
							}
						}
						if (byPlusOne < hcbs) {
							sbjbj = rasterOrderToCodedOrderMappingYPlusOne[bx];
					
							sbj = sbjbj >>> 4;
							bj = sbjbj & 0xF;

							if (bCoded[sbj][bj] == 0) {
								fx = pbx;
								fy = pby + 6;
								verticalFilter(refp, lflimPointer, fx, fy, wcps);
							}
						}
					}
				}
			}
		}

		if (fType == INTRA_FRAME) {
			System.arraycopy(ref[0], 0, goldRef[0], 0, ref[0].length);
			System.arraycopy(ref[1], 0, goldRef[1], 0, ref[1].length);
			System.arraycopy(ref[2], 0, goldRef[2], 0, ref[2].length);
			
			mbMode = mbModeBuffer;
			bCoded = bCodedBuffer;
			cbIndex = cbIndexBuffer;
		}
		internFrameCount++;
		internFrameCount %= maximumPixelSize;
	}
	
	private void coefficientTokenDecode(short[] coeffs, byte[] nCoeffs, byte[] tis, int token, int bi, int ti) throws IOException, EndOfPacketException {
		int sign; // A flag indicating the sign of the current coefficient.
		int mag; // The magnitude of the current coefficient.
		int rlen; // The length of the current ZERO_BYTES run.
		int extraBits = TOKEN_TABLE7_33[0][token];
		int offset = TOKEN_TABLE7_33[1][token];
		int tokenRange = TOKEN_TABLE7_33[2][token];
		int coeffsValue;
		int coeffsIndex;
		int getBuffer;
		byte tisbi;
		
		if (tokenRange == 1) {
			rlen = get(extraBits);
			coeffsIndex = NATURAL_ORDER_SCAN[rlen + ti];
			coeffsValue = 0;
		} else if (tokenRange == 2) {
			coeffsIndex = ti;
			coeffsValue = offset;
			
			if (extraBits >= 1) {
				sign = get1();
				if (extraBits > 1) {
					coeffsValue += get(extraBits - 1);
				}
				if (sign != 0) {
					coeffsValue = -coeffsValue;
				}
			}
		} else if (tokenRange == 3) {
			coeffsValue = 1;
			coeffsIndex = ti + offset;
			sign = get1();
			if (extraBits > 1) {
				coeffsIndex += get(extraBits - 1);
			}
			if (sign != 0) {
				coeffsValue = -coeffsValue;
			}
			coeffsIndex = NATURAL_ORDER_SCAN[coeffsIndex];
		} else {
			getBuffer = get(2);
			sign = getBuffer >>> 1;
			mag = (getBuffer & 0x1) + 2;
			
			if (offset == 0) {
				coeffsIndex = NATURAL_ORDER_SCAN[ti + 1];
			} else {
				coeffsIndex = NATURAL_ORDER_SCAN[ti + get1() + 2];
			}
			coeffsValue = mag;
			if (sign != 0) {
				coeffsValue = -coeffsValue;
			}
		}
		
		rlen = coeffsIndex - ti;
		
		if (rlen > 0) {
			System.arraycopy(ZERO_SHORTS, 0, coeffs, ti, rlen);
		}
		tisbi = tis[bi];
		if ((tisbi += rlen + 1) >= 64) {
			if (tisbi > 64) {
				tisbi = 64;
			}
			qcbIndex = remove(qcbIndex);
		}
		tis[bi] = tisbi;
		coeffs[coeffsIndex] = (short) coeffsValue;
		if (tokenRange != 1) {
			nCoeffs[bi] = tisbi;
		}
	}
	
	private void runBitStringDecode(int nBits, byte[] bits, byte[] huffmanCodeTableRstart, byte[] huffmanCodeTableRbits) throws IOException, EndOfPacketException {
		
		// 7.2 Run-Length Encoded Bit Strings
		// 7.2.1 Long-Run Bit String Decode
		// 7.2.2 Short-Run Bit String Decode
		
		int len = 0; // The number of bits decoded so far. // Porting problem: Should be long
		int bit; // The value associated with the current run.
		int rlen; // The length of the current run.
		int rbits; // The number of extra bits needed to decode the run length.
		//int rstart; // The start of the possible run-length values for a given Huffman code.
		//int roffs; // The offset from RSTART of the runlength.
		int index;
		
		if (len == nBits) {
			return;
		}

		bit = get1();
		
		for (; ; ) {
			index = getHuffcodeTableIndex(huffmanCodeTableRstart.length);
			rbits = huffmanCodeTableRbits[index];
			rlen = huffmanCodeTableRstart[index] + get(rbits);
						
			if (bit == 1) {
				System.arraycopy(oneBytes, 0, bits, len, rlen);
			} else {
				System.arraycopy(zeroBytes, 0, bits, len, rlen);
			}

			len += rlen;
			if (len >= nBits) {
				return;
			}
			if (rlen == 4129) {
				bit = get1();
			} else {
				bit ^= 1;
			}
		}
	}
	
	private int getHuffcodeTableIndex(int length) throws IOException, EndOfPacketException {
		int lengthMinusOne = length - 1;
		int ilog = 0;
		int i;
		
		for (i = 0; i < length; i++) {
			if (get1() == 1) {
				if (++ilog == lengthMinusOne) {
					return ilog;
				}
			} else {
				break;
			}
		}
		return ilog;
	}
	
	private void motionVectorDecode(byte[] mv, int mvMode) throws IOException, EndOfPacketException {
		
		// 7.5.1 Motion Vector Decode
		
		// int mvSign; // The sign of the motion vector component just decoded.
		int bitPattern;
		int mvx, mvy;
		
		if (mvMode == 0) {
			mvx = HUFFMANCODETABLE7_23[0][get(3)];
			if (mvx < 0) {
				mvx = -mvx;
				if (mvx == 2) {
					bitPattern = get1();
				} else {
					bitPattern = get(mvx);
				}
				mvx = HUFFMANCODETABLE7_23[mvx][bitPattern];
			}
			mvx -= 31;
			
			mvy = HUFFMANCODETABLE7_23[0][get(3)];
			if (mvy < 0) {
				mvy = -mvy;
				if (mvy == 2) {
					bitPattern = get1();
				} else {
					bitPattern = get(mvy);
				}
				mvy = HUFFMANCODETABLE7_23[mvy][bitPattern];
			}
			mvy -= 31;
			
		} else {
			mvx = get(5);
			if (get1() == 1) {
				mvx = -mvx;
			}
			mvy = get(5);
			if (get1() == 1) {
				mvy = -mvy;
			}
		}

		mv[0] = (byte) mvx;
		mv[1] = (byte) mvy;
	}


	private void intraPredictor(short[] image, int[] clipW, int pbx, int pby, boolean sparseDc) {

		// 7.9.1 Predictors

		int rpw = clipW.length - PLANE_CLIP_RANGE; // The width of the current plane of the reference frame in pixels.
		int offset = pby * rpw + pbx;
		
		if (sparseDc) {
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset       , 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			System.arraycopy(ONE_HUNDRED_TWENTYEIGHT_INTS, 0, image, offset += rpw, 8);
			return;
		}
		int j = 0;
		int clipMinPlus128 = 128 + CLIP_MIN;

		for (int i = 0; i < 8; i++) {
			image[offset    ] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 1] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 2] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 3] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 4] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 5] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 6] = clip[dqc[j++] + clipMinPlus128];
			image[offset | 7] = clip[dqc[j++] + clipMinPlus128];
			offset += rpw;
		}
	}
	
	private void wholePixelPredictor(short[] image, short[] refp, int[] clipW, int[] clipH, int pbx, int pby, boolean sparseDc) {
				
		// 7.9.1 Predictors

		int rpw = clipW.length - PLANE_CLIP_RANGE; // The width of the current plane of the reference frame in pixels.
		int offset = pby * rpw + pbx;
		
		if (sparseDc) {
			System.arraycopy(refp, offset       , image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			System.arraycopy(refp, offset += rpw, image, offset, 8);
			return;
		}
		int j = 0;
		
		for (int i = 0; i < 8; i++) {
			image[offset    ] = clip[dqc[j++] + refp[offset    ] + CLIP_MIN];
			image[offset | 1] = clip[dqc[j++] + refp[offset | 1] + CLIP_MIN];
			image[offset | 2] = clip[dqc[j++] + refp[offset | 2] + CLIP_MIN];
			image[offset | 3] = clip[dqc[j++] + refp[offset | 3] + CLIP_MIN];
			image[offset | 4] = clip[dqc[j++] + refp[offset | 4] + CLIP_MIN];
			image[offset | 5] = clip[dqc[j++] + refp[offset | 5] + CLIP_MIN];
			image[offset | 6] = clip[dqc[j++] + refp[offset | 6] + CLIP_MIN];
			image[offset | 7] = clip[dqc[j++] + refp[offset | 7] + CLIP_MIN];
			offset += rpw;
		}
	}

	private void wholePixelPredictor(short[] image, short[] refp, int[] clipW, int[] clipH, int mvx, int mvy, int pbx, int pby, boolean sparseDc) {
		
		// 7.9.1 Predictors

		int rpw = clipW.length - PLANE_CLIP_RANGE; // The width of the current plane of the reference frame in pixels.
		int offset = pby * rpw + pbx;
		int pbyPlusMvyPlusPlaneClipMin = pby + mvy + PLANE_CLIP_MIN;
		int offsetXPlusPlaneClipMin = pbx + mvx + PLANE_CLIP_MIN;
		int offsetY;
		if (sparseDc) {
			for (int i = 0; i < 8; i++) {
				offsetY = clipH[pbyPlusMvyPlusPlaneClipMin + i] * rpw;
				image[offset    ] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin    ]] + CLIP_MIN];
				image[offset | 1] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 1]] + CLIP_MIN];
				image[offset | 2] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 2]] + CLIP_MIN];
				image[offset | 3] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 3]] + CLIP_MIN];
				image[offset | 4] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 4]] + CLIP_MIN];
				image[offset | 5] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 5]] + CLIP_MIN];
				image[offset | 6] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 6]] + CLIP_MIN];
				image[offset | 7] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 7]] + CLIP_MIN];
				offset += rpw;
			}
			return;
		}
		int j = 0;
		
		for (int i = 0; i < 8; i++) {
			offsetY = clipH[pbyPlusMvyPlusPlaneClipMin + i] * rpw;
			image[offset    ] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin    ]] + dqc[j++] + CLIP_MIN];
			image[offset | 1] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 1]] + dqc[j++] + CLIP_MIN];
			image[offset | 2] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 2]] + dqc[j++] + CLIP_MIN];
			image[offset | 3] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 3]] + dqc[j++] + CLIP_MIN];
			image[offset | 4] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 4]] + dqc[j++] + CLIP_MIN];
			image[offset | 5] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 5]] + dqc[j++] + CLIP_MIN];
			image[offset | 6] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 6]] + dqc[j++] + CLIP_MIN];
			image[offset | 7] = clip[refp[offsetY + clipW[offsetXPlusPlaneClipMin + 7]] + dqc[j++] + CLIP_MIN];
			offset += rpw;
		}
	}
	
	private void halfPixelPredictor(short[] image, short[] refp, int[] clipW, int[] clipH, int mvx, int mvy, int mvx2, int mvy2, int pbx, int pby, boolean sparseDc) {
		
		// 7.9.1 Predictors
		
		int rpw = clipW.length - PLANE_CLIP_RANGE; // The width of the current plane of the reference frame in pixels.
		int offset = pby * rpw + pbx;
		int pbyPlusMvyPlusPlaneClipMin = pby + mvy + PLANE_CLIP_MIN;
		int pbyPlusMvy2PlusPlaneClipMin = pby + mvy2 + PLANE_CLIP_MIN;
		int offsetXPlusPlaneClipMin = pbx + mvx + PLANE_CLIP_MIN;
		int offsetX2PlusPlaneClipMin = pbx + mvx2 + PLANE_CLIP_MIN;
		int offsetY;
		int offsetY2;
		if (sparseDc) {
			for (int i = 0; i < 8; i++) {
				offsetY = clipH[pbyPlusMvyPlusPlaneClipMin + i] * rpw;
				offsetY2 = clipH[pbyPlusMvy2PlusPlaneClipMin + i] * rpw;
				image[offset    ] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin    ]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin    ]] >> 1) + CLIP_MIN];
				image[offset | 1] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 1]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 1]] >> 1) + CLIP_MIN];
				image[offset | 2] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 2]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 2]] >> 1) + CLIP_MIN];
				image[offset | 3] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 3]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 3]] >> 1) + CLIP_MIN];
				image[offset | 4] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 4]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 4]] >> 1) + CLIP_MIN];
				image[offset | 5] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 5]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 5]] >> 1) + CLIP_MIN];
				image[offset | 6] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 6]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 6]] >> 1) + CLIP_MIN];
				image[offset | 7] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 7]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 7]] >> 1) + CLIP_MIN];
				offset += rpw;
			}
			return;
		}
		int j = 0;

		for (int i = 0; i < 8; i++) {
			offsetY = clipH[pbyPlusMvyPlusPlaneClipMin + i] * rpw;
			offsetY2 = clipH[pbyPlusMvy2PlusPlaneClipMin + i] * rpw;
			image[offset    ] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin    ]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin    ]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 1] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 1]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 1]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 2] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 2]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 2]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 3] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 3]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 3]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 4] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 4]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 4]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 5] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 5]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 5]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 6] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 6]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 6]] >> 1) + dqc[j++] + CLIP_MIN];
			image[offset | 7] = clip[(refp[offsetY + clipW[offsetXPlusPlaneClipMin + 7]] + refp[offsetY2 + clipW[offsetX2PlusPlaneClipMin + 7]] >> 1) + dqc[j++] + CLIP_MIN];
			offset += rpw;
		}

	}
	
	private void dequantization(short[] qmat0, short[] qmat, short[] coeffs, int nCoeffs) {
		
		// 7.9.2 Dequantization
		
		int zzi = 1; // The DCT coefficient index in zig-zag order
				 // in opposite to the DCT coefficient index in natural order
				 // == {int ci = ZIG_ZAG_SCAN[zzi];}.
		
		dqc[0] = (short) (coeffs[0] * qmat0[0]);
		
		for (; zzi < nCoeffs; zzi++) {
			dqc[INVERSE_ZIG_ZAG_SCAN[zzi]] = (short) (coeffs[zzi] * qmat[zzi]);
		}
		for (; zzi < 64; zzi++) {
			dqc[INVERSE_ZIG_ZAG_SCAN[zzi]] = 0;
		}
	}
	
	private void inverseDCT1D4Row(int shift) {
		
		// 7.9.3 The Inverse DCT
		
		// int t0, t1, t2, t3, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		// int r; // A temporary value.
		
		int t0 = C4 * y0 >> 16;
		int t2 = C6 * y2 >> 16;
		int t3 = S6 * y2 >> 16;
		int t4 = C7 * y1 >> 16;
		int t5 = -(S3 * y3 >> 16);
		int t6 = C3 * y3 >> 16;
		int t7 = S7 * y1 >> 16;
		int r = t4 + t5;
		t5 = (short) (t4 - t5);
		t5 = C4 * t5 >> 16;
		t4 = r;
		r = t7 + t6;
		t6 = (short) (t7 - t6);
		t6 = C4 * t6 >> 16;
		t7 = r;
		int t1 = t0 + t2;
		t2 = t0 - t2;
		r = t0 + t3;
		t3 = t0 - t3;
		t0 = r;
		r = t6 + t5;
		t5 = t6 - t5;
		t6 = r;
		dqc[shift    ] = (short) (t0 + t7);
		dqc[shift | 1] = (short) (t1 + t6);
		dqc[shift | 2] = (short) (t2 + t5);
		dqc[shift | 3] = (short) (t3 + t4);
		dqc[shift | 4] = (short) (t3 - t4);
		dqc[shift | 5] = (short) (t2 - t5);
		dqc[shift | 6] = (short) (t1 - t6);
		dqc[shift | 7] = (short) (t0 - t7);
	}
	
	private void inverseDCT1D3Row(int shift) {
		
		// 7.9.3 The Inverse DCT
		
		// int t0, t1, t2, t3, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		// int r; // A temporary value.
		
		int t0 = C4 * y0 >> 16;
		int t2 = C6 * y2 >> 16;
		int t3 = S6 * y2 >> 16;
		int t4 = C7 * y1 >> 16;
		int t7 = S7 * y1 >> 16;
		int t5 = C4 * t4 >> 16;
		int t6 = C4 * t7 >> 16;
		int t1 = t0 + t2;
		t2 = t0 - t2;
		int r = t0 + t3;
		t3 = t0 - t3;
		t0 = r;
		r = t6 + t5;
		t5 = t6 - t5;
		t6 = r;
		dqc[shift    ] = (short) (t0 + t7);
		dqc[shift | 1] = (short) (t1 + t6);
		dqc[shift | 2] = (short) (t2 + t5);
		dqc[shift | 3] = (short) (t3 + t4);
		dqc[shift | 4] = (short) (t3 - t4);
		dqc[shift | 5] = (short) (t2 - t5);
		dqc[shift | 6] = (short) (t1 - t6);
		dqc[shift | 7] = (short) (t0 - t7);
	}
	
	private void inverseDCT1D2Row(int shift) {
		
		// 7.9.3 The Inverse DCT
		
		// int t0, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		// int r; // A temporary value.
		
		int t0 = C4 * y0 >> 16;
		int t4 = C7 * y1 >> 16;
		int t7 = S7 * y1 >> 16;
		int t5 = C4 * t4 >> 16;
		int t6 = C4 * t7 >> 16;
		int r = t6 + t5;
		t5 = t6 - t5;
		t6 = r;
		dqc[shift    ] = (short) (t0 + t7);
		dqc[shift | 1] = (short) (t0 + t6);
		dqc[shift | 2] = (short) (t0 + t5);
		dqc[shift | 3] = (short) (t0 + t4);
		dqc[shift | 4] = (short) (t0 - t4);
		dqc[shift | 5] = (short) (t0 - t5);
		dqc[shift | 6] = (short) (t0 - t6);
		dqc[shift | 7] = (short) (t0 - t7);
	}

	private void inverseDCT1D1Row(int shift) {
		
		// 7.9.3 The Inverse DCT
		
		int t0 = C4 * y0 >> 16;
		
		dqc[shift    ] = t0;
		dqc[shift | 1] = t0;
		dqc[shift | 2] = t0;
		dqc[shift | 3] = t0;
		dqc[shift | 4] = t0;
		dqc[shift | 5] = t0;
		dqc[shift | 6] = t0;
		dqc[shift | 7] = t0;
	}
	
	private void inverseDCT2D(int maxRow) {
		
		// 7.9.3 The Inverse DCT
		
		int ci; // The column index.
		int ri; // The row index.
		int shift;
		int y0, y1, y2, y3, y4, y5, y6, y7;
		int t0, t1, t2, t3, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		int r; // A temporary value.
		
		for (ri = 0; ri <= maxRow; ri++) {
			shift = ri << 3;
			y0 = dqc[shift    ];
			y1 = dqc[shift | 1];
			y2 = dqc[shift | 2];
			y3 = dqc[shift | 3];
			y4 = dqc[shift | 4];
			y5 = dqc[shift | 5];
			y6 = dqc[shift | 6];
			y7 = dqc[shift | 7];
			t0 = (short) (y0 + y4);
			t0 = C4 * t0 >> 16;
			t1 = (short) (y0 - y4);
			t1 = C4 * t1 >> 16;
			t2 = (C6 * y2 >> 16) - (S6 * y6 >> 16);
			t3 = (S6 * y2 >> 16) + (C6 * y6 >> 16);
			t4 = (C7 * y1 >> 16) - (S7 * y7 >> 16);
			t5 = (C3 * y5 >> 16) - (S3 * y3 >> 16);
			t6 = (S3 * y5 >> 16) + (C3 * y3 >> 16);
			t7 = (S7 * y1 >> 16) + (C7 * y7 >> 16);
			r = t4 + t5;
			t5 = (short) (t4 - t5);
			t5 = C4 * t5 >> 16;
			t4 = r;
			r = t7 + t6;
			t6 = (short) (t7 - t6);
			t6 = C4 * t6 >> 16;
			t7 = r;
			r = t0 + t3;
			t3 = t0 - t3;
			t0 = r;
			r = t1 + t2;
			t2 = t1 - t2;
			t1 = r;
			r = t6 + t5;
			t5 = t6 - t5;
			t6 = r;
			dqc[shift    ] = (short) (t0 + t7);
			dqc[shift | 1] = (short) (t1 + t6);
			dqc[shift | 2] = (short) (t2 + t5);
			dqc[shift | 3] = (short) (t3 + t4);
			dqc[shift | 4] = (short) (t3 - t4);
			dqc[shift | 5] = (short) (t2 - t5);
			dqc[shift | 6] = (short) (t1 - t6);
			dqc[shift | 7] = (short) (t0 - t7);
		}
		for (ci = 0; ci < 8; ci++) {
			y0 = dqc[ci     ];
			y1 = dqc[ 8 | ci];
			y2 = dqc[16 | ci];
			y3 = dqc[24 | ci];
			y4 = dqc[32 | ci];
			y5 = dqc[40 | ci];
			y6 = dqc[48 | ci];
			y7 = dqc[56 | ci];
			t0 = (short) (y0 + y4);
			t0 = C4 * t0 >> 16;
			t1 = (short) (y0 - y4);
			t1 = C4 * t1 >> 16;
			t2 = (C6 * y2 >> 16) - (S6 * y6 >> 16);
			t3 = (S6 * y2 >> 16) + (C6 * y6 >> 16);
			t4 = (C7 * y1 >> 16) - (S7 * y7 >> 16);
			t5 = (C3 * y5 >> 16) - (S3 * y3 >> 16);
			t6 = (S3 * y5 >> 16) + (C3 * y3 >> 16);
			t7 = (S7 * y1 >> 16) + (C7 * y7 >> 16);
			r = t4 + t5;
			t5 = (short) (t4 - t5);
			t5 = C4 * t5 >> 16;
			t4 = r;
			r = t7 + t6;
			t6 = (short) (t7 - t6);
			t6 = C4 * t6 >> 16;
			t7 = r;
			r = t0 + t3;
			t3 = t0 - t3;
			t0 = r;
			r = t1 + t2;
			t2 = t1 - t2;
			t1 = r;
			r = t6 + t5;
			t5 = t6 - t5;
			t6 = r;
			dqc[     ci] = (short) (t0 + t7) + 8 >> 4;
			dqc[8  | ci] = (short) (t1 + t6) + 8 >> 4;
			dqc[16 | ci] = (short) (t2 + t5) + 8 >> 4;
			dqc[24 | ci] = (short) (t3 + t4) + 8 >> 4;
			dqc[32 | ci] = (short) (t3 - t4) + 8 >> 4;
			dqc[40 | ci] = (short) (t2 - t5) + 8 >> 4;
			dqc[48 | ci] = (short) (t1 - t6) + 8 >> 4;
			dqc[56 | ci] = (short) (t0 - t7) + 8 >> 4;
		}
	}
	
	private void inverseDCT2D3() {
		
		// 7.9.3 The Inverse DCT
		
		int ci; // The column index.
		int ri; // The row index.
		int shift = 0;
		int t0, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		int r; // A temporary value.
			
		y0 = dqc[0];
		y1 = dqc[1];
		inverseDCT1D2Row(shift);
		shift = 8;
		y0 = dqc[shift    ];
		inverseDCT1D1Row(shift);
		
		for (ci = 0; ci < 8; ci++) {
			int y0 = dqc[     ci];
			int y1 = dqc[ 8 | ci];
			t0 = C4 * y0 >> 16;
			t4 = C7 * y1 >> 16;
			t7 = S7 * y1 >> 16;
			t5 = C4 * t4 >> 16;
			t6 = C4 * t7 >> 16;
			r = t6 + t5;
			t5 = t6 - t5;
			t6 = r;
			dqc[     ci] = (short) (t0 + t7) + 8 >> 4;
			dqc[8  | ci] = (short) (t0 + t6) + 8 >> 4;
			dqc[16 | ci] = (short) (t0 + t5) + 8 >> 4;
			dqc[24 | ci] = (short) (t0 + t4) + 8 >> 4;
			dqc[32 | ci] = (short) (t0 - t4) + 8 >> 4;
			dqc[40 | ci] = (short) (t0 - t5) + 8 >> 4;
			dqc[48 | ci] = (short) (t0 - t6) + 8 >> 4;
			dqc[56 | ci] = (short) (t0 - t7) + 8 >> 4;
		}
	}

	private void inverseDCT2D6() {
		
		// 7.9.3 The Inverse DCT
		
		int ci; // The column index.
		int ri; // The row index.
		int shift = 0;
		int t0, t1, t2, t3, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		int r; // A temporary value.
		
		y0 = dqc[0];
		y1 = dqc[1];
		y2 = dqc[2];
		inverseDCT1D3Row(shift);
		shift = 8;
		y0 = dqc[shift    ];
		y1 = dqc[shift | 1];
		inverseDCT1D2Row(shift);
		shift = 16;
		y0 = dqc[shift    ];
		inverseDCT1D1Row(shift);
		
		for (ci = 0; ci < 8; ci++) {
			int y1 = dqc[ 8 | ci];
			int y2 = dqc[16 | ci];
			t0 = C4 * dqc[ci] >> 16;
			t2 = C6 * y2 >> 16;
			t3 = S6 * y2 >> 16;
			t4 = C7 * y1 >> 16;
			t7 = S7 * y1 >> 16;
			t5 = C4 * t4 >> 16;
			t6 = C4 * t7 >> 16;
			t1 = t0 + t2;
			t2 = t0 - t2;
			r = t0 + t3;
			t3 = t0 - t3;
			t0 = r;
			r = t6 + t5;
			t5 = t6 - t5;
			t6 = r;
			dqc[     ci] = (short) (t0 + t7) + 8 >> 4;
			dqc[8  | ci] = (short) (t1 + t6) + 8 >> 4;
			dqc[16 | ci] = (short) (t2 + t5) + 8 >> 4;
			dqc[24 | ci] = (short) (t3 + t4) + 8 >> 4;
			dqc[32 | ci] = (short) (t3 - t4) + 8 >> 4;
			dqc[40 | ci] = (short) (t2 - t5) + 8 >> 4;
			dqc[48 | ci] = (short) (t1 - t6) + 8 >> 4;
			dqc[56 | ci] = (short) (t0 - t7) + 8 >> 4;
		}
	}

	private void inverseDCT2D10() {
		
		// 7.9.3 The Inverse DCT
		
		int ci; // The column index.
		int ri; // The row index.
		int shift = 0;
		int t0, t1, t2, t3, t4, t5, t6, t7; // An 8-element array containing the current value of each signal line.
		int r; // A temporary value.

		y0 = dqc[0];
		y1 = dqc[1];
		y2 = dqc[2];
		y3 = dqc[3];
		inverseDCT1D4Row(shift);
		shift = 8;
		y0 = dqc[shift    ];
		y1 = dqc[shift | 1];
		y2 = dqc[shift | 2];
		inverseDCT1D3Row(shift);
		shift = 16;
		y0 = dqc[shift    ];
		y1 = dqc[shift | 1];
		inverseDCT1D2Row(shift);
		shift = 24;
		y0 = dqc[shift    ];
		inverseDCT1D1Row(shift);
		
		for (ci = 0; ci < 8; ci++) {
			int y1 = dqc[ 8 | ci];
			int y2 = dqc[16 | ci];
			int y3 = dqc[24 | ci];
			t0 = C4 * dqc[ci] >> 16;
			t2 = C6 * y2 >> 16;
			t3 = S6 * y2 >> 16;
			t4 = C7 * y1 >> 16;
			t5 = -(S3 * y3 >> 16);
			t6 = C3 * y3 >> 16;
			t7 = S7 * y1 >> 16;
			r = t4 + t5;
			t5 = (short) (t4 - t5);
			t5 = C4 * t5 >> 16;
			t4 = r;
			r = t7 + t6;
			t6 = (short) (t7 - t6);
			t6 = C4 * t6 >> 16;
			t7 = r;
			t1 = t0 + t2;
			t2 = t0 - t2;
			r = t0 + t3;
			t3 = t0 - t3;
			t0 = r;
			r = t6 + t5;
			t5 = t6 - t5;
			t6 = r;
			dqc[     ci] = (short) (t0 + t7) + 8 >> 4;
			dqc[8  | ci] = (short) (t1 + t6) + 8 >> 4;
			dqc[16 | ci] = (short) (t2 + t5) + 8 >> 4;
			dqc[24 | ci] = (short) (t3 + t4) + 8 >> 4;
			dqc[32 | ci] = (short) (t3 - t4) + 8 >> 4;
			dqc[40 | ci] = (short) (t2 - t5) + 8 >> 4;
			dqc[48 | ci] = (short) (t1 - t6) + 8 >> 4;
			dqc[56 | ci] = (short) (t0 - t7) + 8 >> 4;
		}
	}
	
	private void horizontalFilter(short[] recp, short[] lflim, int fx, int fy, int rpw){
		
		// 7.10.1 Horizontal Filter
		
		int r; // The edge detector response.
		int by; // The vertical pixel index in the block.
		int pi = fy * rpw + fx; // Pixelindex.
		int piPlus1 = pi + 1;
		int piPlus2 = pi + 2;
		
		for (by = 0; by < 8; by++) {
			r = recp[pi] - 3 * recp[piPlus1] + 3 * recp[piPlus2] - recp[pi + 3] + 4 >> 3;
			recp[piPlus1] = clip[recp[piPlus1] + lflim[r + FILTER_MIN] + CLIP_MIN];
			recp[piPlus2] = clip[recp[piPlus2] - lflim[r + FILTER_MIN] + CLIP_MIN];
			pi += rpw;
			piPlus1 = pi + 1;
			piPlus2 = pi + 2;
		}
	}
	
	private void verticalFilter(short[] recp, short[] lflim, int fx, int fy, int rpw){
		
		// 7.10.2 Vertical Filter
		
		int r; // The edge detector response.
		int bx; // The horizontal pixel index in the block.
		int pi = fy * rpw + fx; // Pixelcoordinate.
		int piPlusRpw = pi + rpw;
		int piPlus2Rpw = piPlusRpw + rpw;
		int piPlus3Rpw = piPlus2Rpw + rpw;
		int piPlusRpwPlusBx;
		int piPlus2RpwPlusBx;
		
		for (bx = 0; bx < 8; bx++) {
			piPlusRpwPlusBx = piPlusRpw + bx;
			piPlus2RpwPlusBx = piPlus2Rpw + bx;
			r = recp[pi + bx] - 3 * recp[piPlusRpwPlusBx] + 3 * recp[piPlus2RpwPlusBx] - recp[piPlus3Rpw + bx] + 4 >> 3;
			recp[piPlusRpwPlusBx] = clip[recp[piPlusRpwPlusBx] + lflim[r + FILTER_MIN] + CLIP_MIN];
			recp[piPlus2RpwPlusBx] = clip[recp[piPlus2RpwPlusBx] - lflim[r + FILTER_MIN] + CLIP_MIN];
		}
	}

	public boolean display() {
		if (ncbs == 0 || externFrameCount == internFrameCount) {
			return false;
		}

		py = ref[0]; // Y
		pv = ref[1]; // Cb
		pu = ref[2]; // Cr
		
		if (internFrameCount == externFrameCount + 1 && !restart) {
			super.display(pixelOffset, pixelRange);
		} else {
			yBStart[0] = yBStart[1] = yBStart[2] = 0;
			yBEnd[0] = yBLength[0];
			yBEnd[1] = yBEnd[2] = yBLength[1];
			super.display(0, maximumPixelSize);
		}
		externFrameCount = internFrameCount;
		return true;
	}
	
	public final short[][] getYCbCrData() {
		return ref;
	}
	
	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		super.close();
		int i, j, k;
		
		if (image == null) {
			return;
		}
		image[0] = null;
		image[1] = null;
		image[2] = null;
		image = null;
		ref[0] = null;
		ref[1] = null;
		ref[2] = null;
		ref = null;
		goldRef[0] = null;
		goldRef[1] = null;
		goldRef[2] = null;
		goldRef = null;
		qis = null;
		for (i = 0; i < bCoded.length; i++) {
			bCoded[i] = null;
		}
		bCoded = null;
		for (i = 0; i < oneBytesBytes.length; i++) {
			oneBytesBytes[i] = null;
		}
		oneBytesBytes = null;
		cbIndex = null;
		ncbIndex = null;
		cifbIndex = null;
		qiis = null;
		mbMode = null;
		mbiToSbiLmbiMapping = null;
		for (i = 0; i < lflim.length; i++) {
			lflim[i] = null;
		}
		lflim = null;
		nCoeffs = null;
		tis = null;
		
		byte[][] mVectsPointer;
	
		for (i = 0; i < mVects.length; i++) {
			mVectsPointer = mVects[i];
			for (j = 0; j < mVectsPointer.length; j++) {
				mVectsPointer[j] = null;
			}
			mVectsPointer = null;
		}
		mVects = null;
		
		int[][] rasterOrderToCodedOrderMappingPointer;
		
		for (i = 0; i < rasterOrderToCodedOrderMapping.length; i++) {
			rasterOrderToCodedOrderMappingPointer = rasterOrderToCodedOrderMapping[i];
			for (j = 0; j < rasterOrderToCodedOrderMappingPointer.length; j++) {
				rasterOrderToCodedOrderMappingPointer[j] = null;
			}
			rasterOrderToCodedOrderMappingPointer = null;
		}
		rasterOrderToCodedOrderMapping = null;
		for (i = 0; i < coeffs.length; i++) {
			coeffs[i] = null;
		}
		coeffs = null;
		mAlphabet = null;
		dqc = null;
		biToMbiMapping = null;
		lastDc = null;
		spipp = null;
		pli = null;
		coeffsValues = null;
		planeWidthClip[0] = null;
		planeWidthClip[1] = null; // No planeWidthClip[2] = null; because third planeWidthClip array is a reused second one.
		planeWidthClip = null;
		planeHeightClip[0] = null;
		planeHeightClip[1] = null; // No planeHeightClip[2] = null; because third planeHeightClip array is a reused second one.
		planeHeightClip = null;
		blockCoordinateX = null;
		blockCoordinateY = null;
		sbpCodedBits = null;
		sbfCodedBits = null;
		bits = null;
		zeroBytes = null;
		oneBytes = null;
		isNb = null;
		xBLength = null;
	 	yBLength = null;
		yBStart = null;
		yBEnd = null;
		qcbIndex = root;
		while (qcbIndex.unmorphableNext != qcbIndex) {
			Node pointer = qcbIndex.unmorphableNext;
			
			qcbIndex = null;
			qcbIndex = pointer;
		}
		qcbIndex = null;
	}

	private Node rebuildIntraCbIndexQueue(int size, int indices[], Node node) {
		for (int i = 0; i < size; i++) {
			node = put(indices[i], node);
		}
		return node;
	}
	
	private Node put(int i, Node node) {
		Node pointer = node;
		
		while (pointer.index != i) {
			pointer = pointer.unmorphableNext;
		}
		pointer.previous = node;
		pointer.next = pointer;
		
		node.next = pointer;
		node = pointer;
		
		return node;
	}
	
	private Node generate(int i, Node node) {
		Node pointer = new Node();
		
		node.unmorphableNext = pointer;
		node = pointer;
		node.index = i;
		
		return node;
	}
		
	private Node remove(Node node) {
		Node pointer = node.previous;
		
		pointer.next = node.next;
		node = pointer;

		return pointer;
	}
	
	private boolean traversable(Node node) {
		return node.next != node ? true : false;
	}
	
	private class Node {
		Node next = this;
		Node previous;
		Node unmorphableNext = this;
		int index = -1;
	}
}
