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
import java.awt.*;
import java.awt.image.*;

import org.xiph.ogg.*;

/**
 * The <code>Output</code> class provides all necessary Theora video display related methods including
 * the color model of Theora. Until now (Theora 1.1),
 * the two defined color models are only different by the gamma values,
 * which are not used here. They may be used from output devices (monitor, etc).
 *
 * @author	Michael Scheerer
 */
abstract class Output {
	// 4.2 Color Space Conversions and Parameters
	//
 	// YCbCr -> RGB mapping
 	//
	// variables are {rU, bV, bgV, rgU}
	//
	// Formulars 4.1 - 4.6 used, with exception of the later Y handling
	//
	// rU = (255 / 224) * 2 * (1 - Kr)
	// bV = (255 / 224) * 2 * (1 - Kb)
	// bgV = (255 / 224) * 2 * (Kb / Kg) * (1 - Kb)
	// rgU = (255 / 224) * 2 * (Kr / Kg) * (1 - Kr)
 	//
	// where:
	// 1. Y = Kr * R + Kg * G + Kb * B
	// 2. Kr + Kg + Kb = 1
	// 3.
	//
	// 0. Kr = 0.2990, Kg = 0.5870, Kb = 0.1140 // Undefined
	// 1. Kr = 0.2990, Kg = 0.5870, Kb = 0.1140 // Rec. ITU-R BT.470-6 System M,    Gamma = 2.2
	// 2. Kr = 0.2990, Kg = 0.5870, Kb = 0.1140 // Rec. ITU-R BT.470-6 System B, G, Gamma = 2.67 instead 2.8
	// 3. Kr = 0.2990, Kg = 0.5870, Kb = 0.1140 // Reserved
	//
	private final static float YUV_TO_RGB_CONVERSION[] = {
		1280F/802F, 661005F/327680F, -128375F/327680F, -266395F/327680F
	};
	
	final static byte CHROMA444 = 3;
	final static byte CHROMA422 = 2;
	final static byte CHROMA420 = 0;
	
	final static byte INTRA_FRAME = 0;
	
	private final static int CLIP_MIN = 260;
	private final static int CLIP_MAX = 780;
	private final static int CLIP_RANGE = 1040;
	
	private static int clip_16[] = new int[CLIP_RANGE];
	private static int clip_8[] = new int[CLIP_RANGE];
	private static int clip[] = new int[CLIP_RANGE];
	
	private static int Y[] = new int[256 + CLIP_RANGE];
	
	private static int[] rU = new int[256 + CLIP_RANGE];
	private static int[] bV = new int[256 + CLIP_RANGE];
	private static int[] rgU = new int[256 + CLIP_RANGE];
	private static int[] bgV = new int[256 + CLIP_RANGE];
	
	
	private static ColorModel DEFAULT = new DirectColorModel(24, 0xFF0000, 0xFF00, 0xFF);
    
	private VideoReader info;
	private int k, j;
	private int u, v;
	private int r, g, b, lum;
	
	private int dst[];
	private int pixelRegion;
	private int pixelOffset;
	private int yCnt;
	private int uvCnt;
	private int clmCnt;
	private int dstCnt;
	private int lineCount;
	private int pictureRegionX;
	private int pictureRegionY;
	private int pictureRegionW;
	private int pictureRegionH;
	private Rectangle imageRegion;
	private Rectangle nullImageRegion;
	private MemoryImageSource source;
	
	long granulepos;
	int codedPictureHeight;
	int chromaWidth;
	int chromaHeight;
	int chromaFormat;
	int codedPictureWidth;
	
	short[] py, pu, pv;
	
	boolean skippedFrame;
	int fType; // The frame type.
	
	static {
		int i;
		
		for(i = -CLIP_MIN; i < CLIP_MAX; i++) {
			clip_16[i + CLIP_MIN] = i >= 0 ? i <= 255 ? i << 16 : 0xFF0000 : 0;
			clip_8[i + CLIP_MIN] = i >= 0 ? i <= 255 ? i << 8 : 0xFF00 : 0;
			clip[i + CLIP_MIN] = i >= 0 ? i <= 255 ? i : 0xFF : 0;
		}
		for(i = 0; i < 256; i++) {
			Y[i] = (int) (255F / 219F * (i - 16) + CLIP_MIN);
			rU[i] = (int) (YUV_TO_RGB_CONVERSION[0] * (i - 128));
			bV[i] = (int) (YUV_TO_RGB_CONVERSION[1] * (i - 128));
			bgV[i] = (int) (YUV_TO_RGB_CONVERSION[2] * (i - 128));
			rgU[i] = (int) (YUV_TO_RGB_CONVERSION[3] * (i - 128));

		}
	}
	
	/**
	 * Constructs an instance of <code>Decoder</code> with a <code>MediaInformation</code>
	 * object containing all necessary informations about the media source.
	 *
	 * @param info     the <code>VideoReader</code> object containing all
	 *      necessary informations about the media source
	 */
	Output(VideoReader info) {
		
		this.info = info;
		
		codedPictureHeight = info.codedPictureHeight;
		chromaFormat = info.chromaFormat;
		codedPictureWidth = info.codedPictureWidth;
		pictureRegionX = info.pictureRegionX;
		pictureRegionY = info.pictureRegionY;
		pictureRegionW = info.pictureRegionW;
		pictureRegionH = info.pictureRegionH;
		dst = new int[codedPictureWidth * codedPictureHeight];
		imageRegion = new Rectangle(pictureRegionX, pictureRegionY, pictureRegionW, pictureRegionH);
		nullImageRegion = new Rectangle(0, 0, 0, 0);
		source = new MemoryImageSource(codedPictureWidth, codedPictureHeight, DEFAULT, dst, codedPictureWidth, this);
	}
	
	private void progressive420() {
		int yCntBuf, uvCntBuff, dstCntBuf;
		
		for (j = pixelOffset; j < pixelRegion; j += 4) {
	  		lum = Y[py[yCnt]];
			u = pu[uvCnt];
			v = pv[uvCnt];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];
	  		dst[dstCnt] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCnt + codedPictureWidth;
			dstCntBuf = dstCnt - codedPictureWidth;

			lum = Y[py[yCntBuf]];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCnt + 1;
			dstCntBuf = dstCnt + 1;

			lum = Y[py[yCntBuf]];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCntBuf + codedPictureWidth;
			dstCntBuf = dstCntBuf - codedPictureWidth;

			lum = Y[py[yCntBuf]];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			clmCnt += 2;
			yCnt += 2;
			dstCnt += 2;
			uvCnt++;
			if (clmCnt == codedPictureWidth) {
				clmCnt = 0;
				yCnt += codedPictureWidth;
				dstCnt -= 3 * codedPictureWidth;
			}
		}
	}
	
	private void progressive422() {
		int yCntBuf, uvCntBuff, dstCntBuf;
		
		for (j = pixelOffset; j < pixelRegion; j += 4) {
	  		lum = Y[py[yCnt]];
			u = pu[uvCnt];
			v = pv[uvCnt];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];
	  		dst[dstCnt] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCnt + codedPictureWidth;
			dstCntBuf = dstCnt - codedPictureWidth;

			lum = Y[py[yCntBuf]];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCnt + 1;
			dstCntBuf = dstCnt + 1;

			lum = Y[py[yCntBuf]];
			
			u = pu[uvCnt];
			v = pv[uvCnt];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];

			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCntBuf + codedPictureWidth;
			dstCntBuf = dstCntBuf - codedPictureWidth;

			lum = Y[py[yCntBuf]];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			clmCnt += 2;
			yCnt += 2;
			dstCnt += 2;
			uvCnt++;
			if (clmCnt == codedPictureWidth) {
				uvCnt += chromaWidth;
				clmCnt = 0;
				yCnt += codedPictureWidth;
				dstCnt -= 3 * codedPictureWidth;
			}
		}
	}
	
	private void progressive444() {
		int yCntBuf, uvCntBuff, dstCntBuf;
		
		for (j = pixelOffset; j < pixelRegion; j += 4) {
	  		lum = Y[py[yCnt]];
			u = pu[uvCnt];
			v = pv[uvCnt];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];
	  		dst[dstCnt] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCnt + codedPictureWidth;
			dstCntBuf = dstCnt - codedPictureWidth;

			lum = Y[py[yCntBuf]];
			uvCntBuff = uvCnt + chromaWidth;
			u = pu[uvCntBuff];
			v = pv[uvCntBuff];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCnt + 1;
			dstCntBuf = dstCnt + 1;

			lum = Y[py[yCntBuf]];
			uvCnt++;
			u = pu[uvCnt];
			v = pv[uvCnt];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];
			
			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			yCntBuf = yCntBuf + codedPictureWidth;
			dstCntBuf = dstCntBuf - codedPictureWidth;

			lum = Y[py[yCntBuf]];
			uvCntBuff = uvCnt + chromaWidth; // still incremented
			u = pu[uvCntBuff];
			v = pv[uvCntBuff];
			r = rU[u];
			g = rgU[u] + bgV[v];
			b = bV[v];

			dst[dstCntBuf] = clip_16[lum + r] | clip_8[lum + g] | clip[lum + b];
			
			clmCnt += 2;
			yCnt += 2;
			dstCnt += 2;
			uvCnt++;
			if (clmCnt == codedPictureWidth) {
				uvCnt += chromaWidth;
				clmCnt = 0;
				yCnt += codedPictureWidth;
				dstCnt -= 3 * codedPictureWidth;
			}
		}
	}
	
	abstract boolean display();

	void display(int offset, int range) {
		pixelRegion = range;
		pixelOffset = offset;
		clmCnt = range % codedPictureWidth;
		yCnt = offset;
		uvCnt = offset >> 2;
		if (chromaFormat == CHROMA422) {
			uvCnt = offset >> 1;
		} else if (chromaFormat == CHROMA444) {
			uvCnt = offset;
		}
		dstCnt = codedPictureWidth * (codedPictureHeight - offset / codedPictureWidth - 1);
		
		if (chromaFormat == CHROMA420) {
			progressive420();
		} else if (chromaFormat == CHROMA422) {
			progressive422();
		} else {
			progressive444();
		}
	}
	
	/**
	 * Returns the image region boundary to update an image region.
	 * With this boundary the actual image region
	 * of the selected image can be displayed
	 * on a video screen display with more performance
	 * than updating the hole image.
	 *
	 * @return  the image region boundary to update an image region
	 */
	public final Rectangle getImageRegion() {
		if (skippedFrame) {
			return nullImageRegion;
		}
		return imageRegion;
	}
	
	abstract void decodeImage() throws IOException, EndOfMediaException, EndOfPacketException;
	
	public final boolean isKeyFrame() {
		return fType == INTRA_FRAME ? true : false;
  	}
	
	public final ImageProducer synthesis(Packet op) throws IOException, EndOfPacketException {
		info.loadPacket(op.packetBase, op.packet, op.bytes);
		
		synchronized(this) {
			decodeImage();
		}
		
		if(op.granulepos > -1){
			granulepos = op.granulepos;
		} else {
			if (granulepos == -1){
				granulepos = 0;
			} else {
				if (fType == INTRA_FRAME){
					long frames = granulepos & VideoReader.BITMASK[info.keyFrameNumberGranuleShift];
					
					granulepos >>>= info.keyFrameNumberGranuleShift;
					granulepos += frames + 1;
					granulepos <<= info.keyFrameNumberGranuleShift;
				} else {
					granulepos++;
				}
			}
		}
		CropImageFilter cropFilter = new CropImageFilter(pictureRegionX, pictureRegionY, pictureRegionW, pictureRegionH);
		return new FilteredImageSource(source, cropFilter);
	}
	
	public final double granuleTime(long granulePosition) {
		if(granulePosition >= 0) {
			return getFrameCount(granulePosition, info.keyFrameNumberGranuleShift, 0) / info.frameRate;
		}
		return -1;
	}
	
	private static int getFrameCount (long granulePosition, int keyFrameNumberGranuleShift, int versionRevisionNumber) {
		if (versionRevisionNumber > 0) {
			return (int) (granulePosition & VideoReader.BITMASK[keyFrameNumberGranuleShift]) + (int) (granulePosition >>> keyFrameNumberGranuleShift) - 1;
		}
		return (int) (granulePosition & VideoReader.BITMASK[keyFrameNumberGranuleShift]) + (int) (granulePosition >>> keyFrameNumberGranuleShift);
	}

	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		dst = null;
		imageRegion = null;
		nullImageRegion = null;
		py = null;
		pu = null;
		pv = null;
	}
	
	final int get1() throws IOException, EndOfPacketException {
		return info.get1();
	}
	
	final int get(int i) throws IOException, EndOfPacketException {
		return info.get(i);
	}

	final int getCodeWord(int bookNumber) throws IOException, EndOfPacketException {
		return info.getCodeWord(bookNumber);
	}
	
	final static int ilog(int v) {
		return AudioReader.ilog(v);
	}
}


