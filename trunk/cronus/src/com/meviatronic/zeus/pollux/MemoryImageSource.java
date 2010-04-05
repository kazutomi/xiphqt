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

import java.awt.image.*;
import java.util.*;

/**
 * This <code>MemoryImageSource</code> class is an optimized version of the
 * origine <code>MemoryImageSource</code> class.
 *
 * @author Michael Scheerer
 */
final class MemoryImageSource implements ImageProducer {
    private int width;
    private int height;
    private ColorModel model;
    private Object pixels;
    private int pixelscan;
	private Output output;
    
    MemoryImageSource(int w, int h, ColorModel cm, Object pix, int scan, Output out) {
		width = w;
		height = h;
		model = cm;
		pixels = pix;
		pixelscan = scan;
		output = out;
    }
	
    public void addConsumer(ImageConsumer ic) {
    }

    public boolean isConsumer(ImageConsumer ic) {
        return false;
    }

    public void removeConsumer(ImageConsumer ic) {
    }

    public void requestTopDownLeftRightResend(ImageConsumer ic) {
    }

    public void startProduction(ImageConsumer ic) {
		try {
			synchronized(output) {
				if(!output.display()) {
					return;
				}
		 	}
			ic.setDimensions(width, height);
			ic.setColorModel(model);
			ic.setHints(ImageConsumer.TOPDOWNLEFTRIGHT | ImageConsumer.COMPLETESCANLINES | ImageConsumer.SINGLEFRAME | ImageConsumer.SINGLEPASS);
			ic.imageComplete(ImageConsumer.STATICIMAGEDONE);
			sendPixels(ic, 0, 0, width, height);
		} catch (Exception e) {
		}
    }

    private void sendPixels(ImageConsumer ic, int x, int y, int w, int h) {
		int off = pixelscan * y + x;
		ic.setPixels(x, y, w, h, model, ((int[]) pixels), off, pixelscan);
    }
}


