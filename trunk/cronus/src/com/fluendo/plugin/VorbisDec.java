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
import com.meviatronic.zeus.castor.*;
import com.fluendo.jst.*;
import com.fluendo.utils.*;

/**
 * @author Fluendo, Michael Scheerer (Adaptions, Fixes)
 */

public class VorbisDec extends Element implements OggPayload
{
  	private long packet;
  	private long offset;
  	private AudioReader vi;
  	private static VorbisDecoder vd;
  	private boolean discont;
	private static Packet op;
	static byte[] convbuffer;
  	
  private static final byte[] signature = { 0x01, 0x76, 0x6f, 0x72, 0x62, 0x69, 0x73 };

  public boolean isType (Packet op)
  {
    return typeFind (op.packetBase, op.packet, op.bytes) > 0;
  }
  public int takeHeader (Packet op) {
		byte header;
	  
		int ret = 0;
		
		try {
			vi.readMediaInformation(op);
		} catch (Exception e) {
			ret = -1;
		}
    	return ret;
  }
  public boolean isHeader (Packet op)
  {
    return (op.packetBase[op.packet] & 0x01) == 0x01;
  }
  public boolean isKeyFrame (Packet op)
  {
    return true;
  }
  public boolean isDiscontinuous ()
  {
    return false;
  }
  public long getFirstTs (Vector packets)
  {
    int len = packets.size();
    int i;
    long total = 0;
    long prevSamples = 0;
    Packet p = new Packet();

    com.fluendo.jst.Buffer buf;

    /* add samples */
    for (i=0; i<len; i++) {
      boolean ignore;
      long temp;

      buf = (com.fluendo.jst.Buffer) packets.elementAt(i);

      p.packetBase = buf.data;
      p.packet = buf.offset;
      p.bytes = buf.length;

      long samples = vi.blocksize(p);
      if (samples <= 0)
        return -1;

      if (prevSamples == 0 ) {
        prevSamples = samples;
        /* ignore first packet */
        ignore = true;
      }
      else
        ignore = false;

      temp = (samples + prevSamples) / 4;
      prevSamples = samples;

      if (!ignore)
        total += temp;
 
      if (buf.time_offset != -1) {
        total = buf.time_offset - total;
	long result = granuleToTime (total);

        buf = (com.fluendo.jst.Buffer) packets.elementAt(0);
	buf.timestamp = result;
        return result;
      }
    }
    return -1;
  }
  public long granuleToTime (long gp)
  {
    if (gp < 0)
      return -1;
	 
    return gp * Clock.SECOND / vi.sampleRate;
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
        case Event.FLUSH_START:
          result = srcPad.pushEvent(event);
	  synchronized (streamLock) {
	    Debug.log(Debug.DEBUG, "synced "+this);
	  }
	  break;
        case Event.FLUSH_STOP:
          result = srcPad.pushEvent(event);
	  break;
        case Event.EOS:
          Debug.log(Debug.INFO, "got EOS "+this);
          result = srcPad.pushEvent(event);
	  break;
        default:
          result = srcPad.pushEvent(event);
	  break;
      }
      return result;
    }
	  
	protected int chainFunc (com.fluendo.jst.Buffer buf) {
    	int result = OK;
      	long timestamp;

      	op.packetBase = buf.data;
      	op.packet = buf.offset;
      	op.bytes = buf.length;
      	op.bos = (packet == 0 ? 1 : 0);
      	op.eos = 0;
      	op.packetno = packet;

      	if (buf.isFlagSet (com.fluendo.jst.Buffer.FLAG_DISCONT)) {
			offset = -1;
			discont = true;
        	Debug.log(Debug.INFO, "vorbis: got discont");
      	}

		if (packet < 3) {
			try {
				vi.readMediaInformation(op);
			} catch (Exception e) {
				// error case; not a vorbis header
	  			Debug.log(Debug.ERROR, "This Ogg bitstream does not contain Vorbis audio data.");
	  			return ERROR;
			}
        	
        	if (packet == 2) {
	  			Debug.log(Debug.INFO, "vorbis rate: "+vi.sampleRate);
				Debug.log(Debug.INFO, "vorbis channels: "+vi.channels);
				
	  			caps = new Caps ("audio/raw");
	  			caps.setFieldInt ("width", 16);
	  			caps.setFieldInt ("depth", 16);
	  			caps.setFieldInt ("rate", vi.sampleRate);
	  			caps.setFieldInt ("channels", vi.channels);
        	}
        	buf.close();
        	packet++;

			return OK;
      } else {
		  	try {
			  	if (vd == null) {
					vd = vi.initializeVorbisDecoder();
					System.out.println(vi.getOggCommentContent());
			  	}
			} catch (Exception e) {}

		  	if (isHeader(op)) {
          		Debug.log(Debug.INFO, "ignoring header");
	  			return OK;
		  	}

        	timestamp = buf.timestamp;
		  
		  	if (timestamp != -1) {
	  			offset = timestamp * vi.sampleRate / Clock.SECOND;
        	} else {
          		timestamp = offset * Clock.SECOND / vi.sampleRate;
			}
	
		  	int samples = vd.getNumberOfSamples() / vi.channels;
		  
		  	try {
				convbuffer = vd.synthesis(op);
			} catch (Exception e) {
				Debug.log(Debug.ERROR, "decoding error");
	  			return ERROR;
			}

		  	int numbytes = 2 * vd.getNumberOfSamples();
		  
	  		buf.ensureSize(numbytes);
	  		buf.offset = 0;
	  		buf.timestamp = timestamp;
	  		buf.time_offset = offset;
	  		buf.length = numbytes;
	  		buf.caps = caps;
	  		buf.setFlag (com.fluendo.jst.Buffer.FLAG_DISCONT, discont);
	  		discont = false;
			System.arraycopy(convbuffer, 2 * vd.getSampleOffset(), buf.data, 0, numbytes);
		  	offset += samples;
		   	if ((result = srcPad.push(buf)) != OK) {
				return ERROR;
			}
	  	}
    	packet++;

    	return result;
	}
};

  	public VorbisDec() {
		super();
	  
	  	if (vi == null) {
		  	vi = new AudioReader();
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
        packet = 0;
		offset = -1;
        break;
      default:
        break;
    }

    res = super.changeState (transition);

    return res;
  }

  public String getFactoryName ()
  {
    return "vorbisdec";
  }
  public String getMime ()
  {
    return "audio/x-vorbis";
  }
  public int typeFind (byte[] data, int offset, int length)
  {
    if (MemUtils.startsWith (data, offset, length, signature))
      return 10;
    return -1;
  }
}
