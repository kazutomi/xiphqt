/* Helena, a Comment decoder created by Michael Scheerer.
 *
 * Helena decoder (c) 2010 Michael Scheerer www.meviatronic.com
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


package com.meviatronic.zeus.helen;

import java.io.*;
import java.util.*;

import com.meviatronic.zeus.castor.*;
import com.meviatronic.zeus.pollux.*;

/**
 * The <code>OggTag</code> class is the main class of this Ogg Comment system.
 * <p>
 * Note that this OggTag system requires an OggTag tag
 * starting at the first byte of the inputstream,
 * although the OggTag specification doesn't demand this.
 *
 * @author    Michael Scheerer
 */
public final class OggTag extends Tag {

	/**
	 * String frameContent keys
	 */
	public final static String
		S_LOCATION = "String location", S_CONTACT = "String contact", S_DESCRIPTION = "String description", S_ORGANIZATION = "String organization",
		S_VERSION = "String version", S_VENDOR = "String vendor", S_LICENSE = "String license", S_DIRECTOR = "String director", S_PRODUCER = "String producer", S_ACTOR = "String actor",
		S_TAG = "String tag";
	
	private boolean framingBit;
	private Hashtable frames;
	private byte[] data;
	private String vendorString;
	private int i, j;
	private int userCommentListLength;
	private Converter con = new Converter();
	private byte[] b;
	private AudioReader reader;

	/**
	 * Provides access to Ogg tag.
	 *
	 * @param reader            AudioReader to read from
	 * @param framingBit        if an framing bit occurs
	 * @exception IOException   if an I/O errors occur
	 */
	public OggTag(AudioReader reader, boolean framingBit) throws IOException {
		super();
		this.framingBit = framingBit;
		this.reader = reader;
	}
	
	private void buildKeyMapTable() {
		//text frameContent
		put(S_ARTIST, "ARTIST");
		put(S_ALBUM, "ALBUM");
		put(S_ENCODER, "ENCODER");
		put(S_TRACK, "TRACKNUMBER");
		put(S_YEAR, "YEAR");
		put(S_LOCATION, "LOCATION");
		put(S_PERFORMER, "PERFORMER");
		put(S_COPYRIGHT, "COPYRIGHT");
		put(S_LICENSE, "LICENSE");
		put(S_DATE, "DATE");
		put(S_GENRE, "GENRE");
		put(S_TITLE, "TITLE");
		put(S_ISRC, "ISRC");
		put(S_VERSION, "VERSION");
		put(S_ORGANIZATION, "ORGANIZATION");
		put(S_CONTACT, "CONTACT");
		put(S_DESCRIPTION, "DESCRIPTION");
	}

	/**
	 * Returns the value to which the specified key is mapped in this hashtable.
	 * The values are either control flags or informations stored in the tag.
	 *
	 * @param key                       the hashtable key
	 * @return                          the value to which the key is mapped in
	 *      this hashtable; null if the key is not mapped to any value in this
	 *      hashtable
	 */
	public Object get(Object key) {
		Object ob = super.get(key);

		if (key.equals(S_TAG_FORMAT) || key.equals(B_TAG) || key.equals(S_VENDOR)) {
			return ob;
		}
		return frames.get(ob);
	}
	
	/**
	 * Obtains a String describing all information keys and values. With the hashkey
	 * of a specific information it is possible to obtain the information value.
	 *
	 * @return     a String representation all informations
	 */
	public String toString() {
		StringBuffer buff = new StringBuffer();
		int counter = 0, counter1 = 0;
		Object key;
		Object value;
		Enumeration ekeys1 = keys();
		if (!ekeys1.hasMoreElements()) {
			buff.append("{}");
			return buff.toString();
		}
		for (key = ekeys1.nextElement(); ekeys1.hasMoreElements(); key = ekeys1.nextElement()) {
			value = get(key);
			if (value != null && !key.equals(S_VENDOR)) {
				counter++;
			}
		}
		Enumeration ekeys = keys();
		buff.append("{");
		buff.append(S_VENDOR + "=");
		if (counter == 0) {
			buff.append(get(S_VENDOR));
		} else {
			buff.append(get(S_VENDOR) + ", ");
		}
		for (key = ekeys.nextElement(); ekeys.hasMoreElements(); key = ekeys.nextElement()) {
			value = get(key);
			if (value != null && !key.equals(S_VENDOR)) {
				buff.append(key + "=");
				buff.append(value);
				counter1++;
				if (counter1 < counter) {
					buff.append(", ");
				}
			}
		}
		buff.append("}");
		return buff.toString();
	}
	
    /**
     * Creates a shallow copy of this hashtable. The keys and values
     * themselves are cloned.
     * This is a relatively expensive operation.
	 *
	 * @return     a clone of this instance
	 */
	public Object clone() {
		BaseTag hash = new BaseTag();
		
		int i = size();
		
		if (i == 0) {
			return hash;
		}

		Enumeration ekeys = keys();
		Enumeration evalues = elements();
		Object key, value;
		for (int j = 0; j < i; j++) {
			key = ekeys.nextElement();
			value = get(key);
			if (value != null) {
				hash.put(key, value);
			}
		}
		return hash;
	}
	
	/*
	 * Decodes the comment header packet
	 * @exception IOException           if an I/O errors occur
	 * @exception EndOfPacketExeption   if an end of packet occur
	 */
	public void decode() throws IOException, EndOfPacketException {
		try {
			readHeader(reader);
			buildKeyMapTable();
			put(S_VENDOR, new String(vendorString));
			loadFrames(reader, framingBit);
		} catch (TagException e) {
			throw new IOException("Tag error");
		}
	}

	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		con = null;
		data = null;
		b = null;
		vendorString = null;
		
		if (frames != null) {
			frames.clear();
		}
		frames = null;
		super.close();
	}
	 
	private void readHeader(AudioReader reader) throws TagException, IOException, EndOfPacketException {
		int vendorLength = getInt();
		
		if(vendorLength < 0) {
			throw new TagException("Negative String Size");
		}

		data = new byte[vendorLength];
		
		for (int i = 0; i < vendorLength; i++) {
			data[i] = (byte) getByte();
		}

		vendorString = new String(data);
		
		userCommentListLength = getInt();
		
		if(userCommentListLength < 0) {
			throw new TagException("Negative Comment List Size");
		}
		
		frames = new Hashtable(userCommentListLength);
	}

	private void loadFrames(AudioReader reader, boolean framingBit) throws IOException, TagException, EndOfPacketException {
		int length = 0;
		
		String key;
		
		String value;
		
		int i, j, k;
		
		String keyBuffer = "";
		
		String valueBuffer = "";
		
		for (i = 0; i < userCommentListLength; i++) {
			length = getInt();
			
			if (length < 0) {
				throw new TagException("Negative String Size");
			}

			data = new byte[length];
			
			for (j = 0; j < length; j++) {
				data[j] = (byte) getByte();
			}
			
			k = 0;
			
			for (; k < data.length; k++) {
				if (data[k] == '=') {
					break;
				}
			}
			key = new String(data, 0, k).toUpperCase();
			value = con.convert(data, k + 1, data.length);
			
			if (key.equals(keyBuffer)) {
				value = valueBuffer+", "+value;
			}
			
			keyBuffer = new String(key);
			valueBuffer = new String(value);
			frames.put(key, value);
		}
		if (framingBit) {
			if (getLittleEndian1() == 0) {
				throw new TagException("No Ogg Tag");
			}
		}
	}
	
	/**
	 * Returns a single bit.
	 *
	 * @return                               the integer value of one bit
	 * @exception IOException                if one bit can't be retrieved
	 * @exception EndOfPacketExeption        if an end of packet occur
	 */
	public int getLittleEndian1() throws IOException, EndOfPacketException {
		return reader.getLittleEndian1();
	}

	/**
	 * Returns a single byte.
	 *
	 * @return                               the integer value of one bit
	 * @exception IOException                if one bit can't be retrieved
	 * @exception EndOfPacketExeption        if an end of packet occur
	 */
	public int getByte() throws IOException, EndOfPacketException {
		if (reader instanceof VideoReader) {
			return ((VideoReader) reader).get(8);
		}
		return reader.getLittleEndian(8);
	}
	
	/**
	 * Returns an int
	 *
	 * @return                               the integer value
	 * @param i                              the length in bits
	 * @exception IOException                if the bits can't be retrieved
	 * @exception EndOfPacketExeption        if an end of packet occur
	 */
	public int getInt() throws IOException, EndOfPacketException {
		if (reader instanceof VideoReader) {
			return ((VideoReader) reader).get(8) | ((VideoReader) reader).get(8) << 8 | ((VideoReader) reader).get(8) << 16 | ((VideoReader) reader).get(8) << 24;
		}
		return reader.getLittleEndian(32);
	}
}


