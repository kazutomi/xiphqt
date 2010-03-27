/* Copyright (C) <2004> Wim Taymans <wim@fluendo.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

package com.fluendo.plugin;

import java.util.*;
import org.xiph.ogg.*;
import com.meviatronic.zeus.pollux.*;
import com.fluendo.jst.*;
import com.fluendo.utils.*;

/**
 * @author Fluendo, Michael Scheerer (Adaptions, Fixes)
 */

public class TheoraDec extends Element implements OggPayload
{
  private static final byte[] signature = { -128, 0x74, 0x68, 0x65, 0x6f, 0x72, 0x61 };

  private VideoReader ti;
  private static TheoraDecoder td;
  private static Packet op;
  private int packet;

  private long lastTs;
  private boolean needKeyframe;
  private boolean haveDecoder;
  private static boolean haveComment;
	
  /*
   * OggPayload interface
   */
  public boolean isType (Packet op)
  {
    return typeFind (op.packetBase, op.packet, op.bytes) > 0;
  }
  public int takeHeader (Packet op)
	{
		byte header;
		
		int ret = 0;
		
		try {
			ti.readMediaInformation(op);
		} catch (Exception e) {
			ret = -1;
		}
		
		header = op.packetBase[op.packet];
		
	if (header == -126 && !haveDecoder) {
		td = ti.initializeTheoraDecoder();
		if (!haveComment) {
			System.out.println(ti.getOggCommentContent());
			haveComment = true;
		}
		haveDecoder = true;
    }
    return ret;
  }
  public boolean isHeader (Packet op)
  {
    return (op.packetBase[op.packet] & 0x80) == 0x80;
  }
  public boolean isKeyFrame (Packet op)
  {
	  return (op.packetBase[op.packet] & 0x40) == 0;
  }
  public boolean isDiscontinuous ()
  {
    return false;
  }
  public long getFirstTs (Vector packets)
  {
    int len = packets.size();
    int i;
    long time;
    com.fluendo.jst.Buffer data = null;

    /* first find buffer with valid offset */
    for (i=0; i<len; i++) {
      data = (com.fluendo.jst.Buffer) packets.elementAt(i);

      if (data.time_offset != -1)
        break;
    }
    if (i == packets.size())
      return -1;

    time = granuleToTime (data.time_offset);

    data = (com.fluendo.jst.Buffer) packets.elementAt(0);
    data.timestamp = time - (long) ((i+1) * (Clock.SECOND / ti.frameRate));

    return time;
  }
  public long granuleToTime (long gp)
  {
    long res;

    if (gp < 0 || !haveDecoder)
      return -1;
	res = (long) (td.granuleTime(gp) * Clock.SECOND);

    return res;
  }

  private Pad srcPad = new Pad(Pad.SRC, "src") {
    protected boolean eventFunc (com.fluendo.jst.Event event) {
      return sinkPad.pushEvent(event);
    }
  };

  private Pad sinkPad = new Pad(Pad.SINK, "sink") {
    protected boolean eventFunc (com.fluendo.jst.Event event) {
      boolean result;

      switch (event.getType()) {
        case com.fluendo.jst.Event.FLUSH_START:
	  result = srcPad.pushEvent (event);
	  synchronized (streamLock) {
            Debug.log(Debug.DEBUG, "synced "+this);
	  }
          break;
        case com.fluendo.jst.Event.FLUSH_STOP:
          result = srcPad.pushEvent(event);
          break;
        case com.fluendo.jst.Event.EOS:
          Debug.log(Debug.INFO, "got EOS "+this);
          result = srcPad.pushEvent(event);
          break;
        case com.fluendo.jst.Event.NEWSEGMENT:
	default:
          result = srcPad.pushEvent(event);
          break;
      }
      return result;
    }

    protected int chainFunc (com.fluendo.jst.Buffer buf) {
    	int result;
      	long timestamp;

      	Debug.log( Debug.DEBUG, parent.getName() + " <<< " + buf );

      	op.packetBase = buf.data;
      	op.packet = buf.offset;
      	op.bytes = buf.length;
      	op.bos = (packet == 0 ? 1 : 0);
      	op.eos = 0;
      	op.packetno = packet;
      	timestamp = buf.timestamp;

      	if (buf.isFlagSet (com.fluendo.jst.Buffer.FLAG_DISCONT)) {
        	Debug.log(Debug.INFO, "theora: got discont");
        	needKeyframe = true;
			lastTs = -1;
	  	}
		
		if (packet < 3) {
		 	if (takeHeader(op) < 0) {
          		buf.close();
          		// error case; not a theora header
          		Debug.log(Debug.ERROR, "does not contain Theora video data.");
          		return ERROR;
        	}
		  	if (packet == 2) {
           		Debug.log(Debug.INFO, "theora dimension: "+ti.codedPictureWidth+"x"+ti.codedPictureHeight);
          		Debug.log(Debug.INFO, "theora offset: "+ti.pictureRegionX+","+ti.pictureRegionY);
          		Debug.log(Debug.INFO, "theora frame: "+ti.pictureRegionW+","+ti.pictureRegionH);
          		Debug.log(Debug.INFO, "theora aspect: "+ti.aspectRatio.width+"/"+ti.aspectRatio.height);
          		Debug.log(Debug.INFO, "theora framerate: "+ti.frameRate);

	  			caps = new Caps ("video/raw");
	  			caps.setFieldInt ("width", ti.pictureRegionW);
	  			caps.setFieldInt ("height", ti.pictureRegionH);
	  			caps.setFieldInt ("aspect_x", ti.aspectRatio.width);
	  			caps.setFieldInt ("aspect_y", ti.aspectRatio.height);
        	}
       	 	buf.close();
        	packet++;
			return OK;
		} else {
			if (op.bytes == 0) {
				Debug.log(Debug.DEBUG, "duplicate frame");
				return OK;
			}
        	if ((op.packetBase[op.packet] & 0x80) == 0x80) {
          		Debug.log(Debug.INFO, "ignoring header");
          		return OK;
			}
			if (needKeyframe && isKeyFrame(op)) {
	  			needKeyframe = false;
        	}

			if (timestamp != -1) {
	  			lastTs = timestamp;
			} else if (lastTs != -1) {
	  			long add;

	  			add = (long) (Clock.SECOND / ti.frameRate);
	  			lastTs += add;
	  			timestamp = lastTs;
			}

			if (!needKeyframe) {
				try{
					buf.object = td.synthesis(op);
          		} catch (Exception e) { // Dirty hack!!!!!!!!
					e.printStackTrace();
					e.getMessage().equals(" ");
	    			postMessage (Message.newError (this, e.getMessage()));
            		result = ERROR;
				}
				buf.caps = caps;
	    		buf.timestamp = timestamp;
            	Debug.log( Debug.DEBUG, parent.getName() + " >>> " + buf );
            	result = srcPad.push(buf);
			} else {
				result = OK;
	  			buf.close();
			}
	 	}
		packet++;

      	return result;
    }

	protected boolean activateFunc (int mode) {
    	return true;
    }
  };

	public TheoraDec() {
    	super();
	  	if (ti == null) {
		  	ti = new VideoReader();
	  	}
	 
	  	if (op == null) {
		  	op = new Packet();
	  	}
	  
    	addPad (srcPad);
	  	addPad (sinkPad);
  	}

  protected int changeState (int transition) {
    int res;

    switch (transition) {
      case STOP_PAUSE:
        lastTs = -1;
        packet = 0;
        needKeyframe = true;
	break;
      default:
        break;
    }

    res = super.changeState (transition);

    switch (transition) {
      case PAUSE_STOP:
	
	break;
      default:
        break;
    }

    return res;
  }

  public String getFactoryName ()
	{
    return "theoradec";
  }
  public String getMime ()
  {
    return "video/x-theora";
  }
  public int typeFind (byte[] data, int offset, int length)
  {
    if (MemUtils.startsWith (data, offset, length, signature))
      return 10;
    return -1;
  }
}
