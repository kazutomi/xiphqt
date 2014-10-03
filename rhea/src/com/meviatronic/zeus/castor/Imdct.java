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

/**
 *  The <code>Imdct</code> class provides an implementation of the inverse MDCT
 *  based on:
 *  The use of multirate filter banks for coding of high quality digital audio,
 *  6th European Signal Processing Conference (EUSIPCO), Amsterdam, June 1992, Vol.1, pages 211-214Th,
 *  Sporer Kh.Brandenburg B.Edler
 *
 *  @author     Michael Scheerer
 */
final class Imdct {
	private float[] A;
	private float[] AR;
	private float[] BH;
	private float[] C;
	private int[] bitreversedIndex;
	private int n;
	private int nHalf;
	private int nQuarter;
	private int nThreeFourths;
	private int nEighth;
	private int lEnd;
 
	void initialize(int n) {
	
		this.n = n;
		
		int i, j, k = 0;
		
		nHalf = n >>> 1;
		nQuarter = n >>> 2;
		nEighth = n >>> 3;
		nThreeFourths = nQuarter * 3;
		A = new float[nHalf];
		AR = new float[nHalf];
		BH = new float[nHalf];
		C = new float[nQuarter];
		bitreversedIndex = new int[nEighth];
	
		int log2n = (int) Math.rint((Math.log(n) / Math.log(2)));
	  
	  	lEnd = log2n - 3;
	  
		int codePattern;
		
		for (i = 0; i < nEighth; i++) {
			codePattern = 0;
			
			for (j = 0; j < lEnd; j++) {
				codePattern <<= 1;
				codePattern |= i >>> j & 1;
			}
			
			j = codePattern;
			
			if (!(i > 0 && i < nEighth - 1)) {
				j = i;
			}
			bitreversedIndex[i] = j;
		}
		
		int nHalfMinusI2Minus1;
		int nHalfMinusI2Minus2;
		int i2, i2Plus1;
		
	  	for (i = 0; i < nQuarter; i++) {
		  	i2 = i << 1;
		  	i2Plus1 = i2 + 1;
			nHalfMinusI2Minus1 = nHalf - i2 - 1;
			nHalfMinusI2Minus2 = nHalf - i2 - 2;
			AR[nHalfMinusI2Minus1] = A[i2] = (float) Math.cos(Math.PI / n * 4 * i);
			AR[nHalfMinusI2Minus2] = A[i2Plus1] = (float) -Math.sin(Math.PI / n * 4 * i);
			BH[i2] = (float) Math.cos(Math.PI / (n << 1) * (i << 1 | 1)) / 2;
			BH[i2Plus1] = (float) Math.sin(Math.PI / (n << 1) * (i << 1 | 1)) / 2;
		}

	  	for (i = 0; i < nEighth; i++) {
		  	i2 = i << 1;
		  	i2Plus1 = i2 + 1;
			C[i2] = (float) Math.cos(Math.PI / nHalf * (i << 1 | 1));
			C[i2Plus1] = (float) -Math.sin(Math.PI / nHalf * (i << 1 | 1));
		}
	}
	
	void decode(float[] in, float[] out, float[] window, float[] floor) {
		int k, l, r, s;
		float upOdd, downOdd, upEven, downEven;
		int upOddIndex, downOddIndex, upEvenIndex, downEvenIndex;
 		int i2k, i2kPlus1, i2kPlusNquarter, i2kPlus1PlusNquarter;
 		int i4k, i4kr, i4kPlus1, i4kPlus3;
		int iNquarterMinuskMinus1, iNthreeFourthsMinuskMinus1, iNthreeFourthsPlusk, iNquarterPlusk;
 		int iNquarterMinus1 = nQuarter - 1, iNthreeFourthsMinus1 = nThreeFourths - 1;
 		int iNhalfMinus1 = nHalf - 1, iNhalfMinus2 = nHalf - 2, iNhalfMinus4 = nHalf - 4;
 		int iNhalfMinus4kMinus4, iNhalfMinus4kMinus4r;
		int iNhalfMinus2Minusr2;
		int k0, k0Half, k1;
		int rEnd, sEnd;
		int rk1;
		float ark1, ark1Plus1;
		float ar2k, ar2kPlus1, ar2kPlusNhalf, ar2kPlus1PlusNhalf, ar4k, ar4kPlus1;
 		float factor1, factor2, factor3, factor4, factor5, factor6;
		float base1, base2;
		float w4k, w4kPlus1, w4kPlus3;
		float wNhalfMinus4kMinus1, wNhalfMinus4kMinus2, wNhalfMinus4kMinus4;
		float c2k, c2kPlus1;
		float bh2k, bh2kPlus1;
		
		// First half:
		// Vn-4k-1 = 2 * (U4k * A2k - U4k+2 * A2k+1)
		// Vn-4k-3 = 2 * (U4k * A2k+1 + U4k+2 * A2k)

		// Second half:
		// Vn-4k-1 = 2 * (-Un-4k-1 * A2k + Un-4k-3 * A2k+1)
		// Vn-4k-3 = 2 * (-Un-4k-1 * A2k+1 - Un-4k-3 * A2k)
		
		// First half mirrored index:
		// V4k+3 = 2 * (Un-4k-4 * Ar2k - Un-4k-2 * Ar2k+1)
		// V4k+1 = 2 * (Un-4k-4 * Ar2k+1 + Un-4k-2 * Ar2k)
		
		// Endresult:
		// First half mirrored index mirrored array:
		// V4k+3 = 2 * (-U4k+3 * Ar2k + U4k+1 * Ar2k+1)
		// V4k+1 = 2 * (-U4k+3 * Ar2k+1 - U4k+1 * Ar2k)

		// Second half mirrored index:
		// V4k+3 = 2 * (-U4k+3 * Ar2k + U4k+1 * Ar2k+1)
		// V4k+1 = 2 * (-U4k+3 * Ar2k+1 - U4k+1 * Ar2k)
		
		// Second half mirrored index plus nHalf:
		// V4k+3+nHalf = 2 * (-U4k+3+nHalf * Ar2k+nHalf + U4k+1+nHalf * Ar2k+1+nHalf)
		// V4k+1+nHalf = 2 * (-U4k+3+nHalf * Ar2k+1+nHalf - U4k+1+nHalf * Ar2k+nHalf)
		
		// Endresult:
		// Second half mirrored index mirrored array plus nHalf:
		// V4k+3+nHalf = 2 * (UnHalf-4k-4 * Ar2k+nHalf - UnHalf-4k-2 * Ar2k+1+nHalf)
		// V4k+1+nHalf = 2 * (UnHalf-4k-4 * Ar2k+1+nHalf + UnHalf-4k-2 * Ar2k+nHalf)
		
		// W4k+3+nHalf = V4k+3+nHalf + V4k+3
		// W4k+1+nHalf = V4k+1+nHalf + V4k+1
		// W4k+3 = (V4k+3+nHalf - V4k+3) * Ar4k - (V4k+1+nHalf - V4k+1) * Ar4k+1
		// W4k+1 = (V4k+1+nHalf - V4k+1) * Ar4k - (V4k+3+nHalf - V4k+3) * Ar4k+1
		
		for (k = 0; k < nEighth; k++) {
			i4k = k << 2;
			i2k = k << 1;
			i2kPlus1 = i2k | 1;
			i4kPlus1 = i4k | 1;
			i4kPlus3 = i4k | 3;
			i2kPlusNquarter = i2k | nQuarter;
			i2kPlus1PlusNquarter = i2kPlus1 | nQuarter;
			iNhalfMinus4kMinus4 = iNhalfMinus4 - i4k;
			w4kPlus1 = in[i4kPlus1] * floor[i4kPlus1];
			w4kPlus3 = in[i4kPlus3] * floor[i4kPlus3];
			wNhalfMinus4kMinus4 = in[iNhalfMinus4kMinus4] * floor[iNhalfMinus4kMinus4];
			wNhalfMinus4kMinus2 = in[iNhalfMinus4kMinus4 | 2] * floor[iNhalfMinus4kMinus4 | 2];
			ar2k = AR[i2k];
			ar2kPlus1 = AR[i2kPlus1];
			ar2kPlusNhalf = AR[i2kPlusNquarter];
			ar2kPlus1PlusNhalf = AR[i2kPlus1PlusNquarter];
			ar4k = AR[i4k];
			ar4kPlus1 = AR[i4kPlus1];
			factor1 = -w4kPlus3 * ar2kPlus1 + w4kPlus1 * ar2k; // V4k+3
			factor2 = -w4kPlus3 * ar2k - w4kPlus1 * ar2kPlus1; // V4k+1
			factor3 = wNhalfMinus4kMinus4 * ar2kPlus1PlusNhalf - wNhalfMinus4kMinus2 * ar2kPlusNhalf; // V4k+3+nHalf
			factor4 = wNhalfMinus4kMinus4 * ar2kPlusNhalf + wNhalfMinus4kMinus2 * ar2kPlus1PlusNhalf; // V4k+1+nHalf
			out[i2kPlus1PlusNquarter] = factor1 + factor3;
			out[i2kPlusNquarter] = factor2 + factor4;
			factor5 = factor3 - factor1;
			factor6 = factor4 - factor2;
			out[i2kPlus1] = factor5 * ar4kPlus1 - factor6 * ar4k;
			out[i2k] =      factor5 * ar4k + factor6 * ar4kPlus1;
		}

		for (l = 0; l < lEnd; l++) {
			k0 = n >>> l + 2;
			k1 = 1 << l + 3;
			k0Half = k0 >>> 1;
			rEnd = k0 >>> 2;
			sEnd = k1 >>> 2;
			for (r = 0; r < rEnd; r++) {
				iNhalfMinus2Minusr2 = iNhalfMinus2 - (r << 1);
				ark1 = A[rk1 = r * k1];
				ark1Plus1 = A[rk1 | 1];
				for (s = 0; s < sEnd; s++) {
					downEvenIndex = iNhalfMinus2Minusr2 - k0 * s;
					downOddIndex = downEvenIndex - k0Half;
					upEven = out[upEvenIndex = downEvenIndex | 1];
					upOdd = out[upOddIndex = downOddIndex | 1];
					downEven = out[downEvenIndex];
					downOdd = out[downOddIndex];
					out[upEvenIndex] = upEven + upOdd;
					out[downEvenIndex] = downEven + downOdd;
					factor5 = upEven - upOdd;
					factor6 = downEven - downOdd;
					out[upOddIndex] =   factor5 * ark1 - factor6 * ark1Plus1;
					out[downOddIndex] = factor5 * ark1Plus1 + factor6 * ark1;
				}
 			}
		}
		
		// Un-2k-1 = W4k
		// Un-2k-2 = W4k+1
		// UnThreeFourths-2k-1 = W4k+2
		// UnThreeFourths-2k-2 = W4k+3

		// Partial mirrored indices:
		// Un-2k-1 = W4k
		// Un-2k-2 = W4k+1
		// U2k+1+nHalf = WnHalf-4k-2
		// U2k+nHalf = WnHalf-4k-1

		// Faktor1 = (WnHalf-4k-1 + W4k+1)
		// Faktor2 = (WnHalf-4k-1 - W4k+1) * C2k+1
		// Faktor3 = (WnHalf-4k-2 + W4k  ) * C2k
		// Faktor4 = (WnHalf-4k-2 - W4k  )
		// Faktor5 = (WnHalf-4k-2 + W4k  ) * C2k+1
		// Faktor6 = (WnHalf-4k-1 - W4k+1) * C2k

		// V2k+nHalf    =  Faktor1+Faktor2+Faktor3
		// Vn-2k-2      =  Faktor1-Faktor2-Faktor3
		// V2k+1+nHalf  =  Faktor4+Faktor5-Faktor6
		// Vn-2k-1      = -Faktor4+Faktor5-Faktor6
		
		for (k = 0; k < nEighth; k++) {
			i4k = k << 2;
			i4kr = bitreversedIndex[k] << 2;
			iNhalfMinus4kMinus4r = iNhalfMinus4 - i4kr;
			i2k = k << 1;
			c2k = C[i2k];
			c2kPlus1 = C[i2k | 1];
			w4k = out[i4kr];
			w4kPlus1 = out[i4kr | 1];
			wNhalfMinus4kMinus1 = out[iNhalfMinus4kMinus4r | 3];
 			wNhalfMinus4kMinus2 = out[iNhalfMinus4kMinus4r | 2];
 			base1 = wNhalfMinus4kMinus1 - w4kPlus1;
			base2 = wNhalfMinus4kMinus2 + w4k;
 			factor1 =  wNhalfMinus4kMinus1 + w4kPlus1;
 			factor2 = base1 * c2kPlus1;
 			factor3 = base2 * c2k;
			factor4 =  wNhalfMinus4kMinus2 - w4k;
			factor5 = base2 * c2kPlus1;
 			factor6 = base1 * c2k;
			in[i2k]					=  factor1 + factor2 + factor3;
			in[iNhalfMinus2 - i2k]   =  factor1 - factor2 - factor3;
			in[i2k | 1]				=  factor4 + factor5 - factor6;
			in[iNhalfMinus1 - i2k]   = -factor4 + factor5 - factor6;
		}
		
		for (k = 0; k < nQuarter; k++) {
			i2k = k << 1;
 			i2kPlus1 = i2k | 1;
 			iNquarterMinuskMinus1 = iNquarterMinus1 - k;
			iNthreeFourthsMinuskMinus1 = iNthreeFourthsMinus1 - k;
 			iNthreeFourthsPlusk = nThreeFourths | k;
			iNquarterPlusk = nQuarter | k;
			bh2k = BH[i2k];
			bh2kPlus1 = BH[i2kPlus1];
			factor5 = in[i2k];
			factor6 = in[i2kPlus1];
			factor1 = factor5 * bh2k + factor6 * bh2kPlus1;
			factor2 = factor5 * bh2kPlus1 - factor6 * bh2k;
			out[iNquarterMinuskMinus1] = factor2 * window[iNquarterMinuskMinus1];
			out[iNthreeFourthsPlusk] = -factor1 * window[iNthreeFourthsPlusk];
			out[iNquarterPlusk] = -factor2 * window[iNquarterPlusk];
 			out[iNthreeFourthsMinuskMinus1] = -factor1 * window[iNthreeFourthsMinuskMinus1];
		 }
	}

 	void close() {
		A = null;
		AR = null;
		BH = null;
		C = null;
		bitreversedIndex = null;
	}
}
