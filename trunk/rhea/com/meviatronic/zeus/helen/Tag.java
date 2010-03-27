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

import java.util.*;
import java.io.*;

/**
 * The <code>Tag</code> class provides methods to obtain the content of media tag
 * informations, which describe possible extra informations of a media source.
 * So it's possible to obtain, add and change tag informations with get and
 * put methods. Predefined base hashkeys are defined in this class,
 * but content related hashkeys must be defined inside derived
 * classes. Note that all derived classes of this class are not used within
 * this library itself. Instead media codec plug-ins of this library uses
 * the tag system. Also the tag system can contain several tag formats like
 * IDv1, IDv2 or Ogg Vorbis Tag, but a plug-in system of these different formats
 * are not supported, because the tag system classes are referenced and instanced
 * directly inside the codecs.
 *
 * <p>
 * All tag related values of the hashkeys are <b>not</b> predefined with default values.
 *
 * <p>
 * To create a hypothetical IDv2 tag plug-in it's required to derive
 * this class according to the following listing:
 *
 * <blockquote>
 * <pre>
 * package org.ljmf.audio.codec.tag.id3v2;<p>
 *
 * import java.io.*;
 * import java.util.*;
 * import java.util.zip.*;<p>
 *
 * import org.ljmf.audio.codec.tag.*;<p>
 *
 * public final class MyIDv2Tag extends Tag {<p>
 *
 *     //====================<p>
 *
 *     public MyIDv2Tag(InputStream stream) throws IOException, TagException {
 *         super();<p>
 *
 *         try {
 *             readHeader(stream);
 *         } catch (TagException e) {
 *             close();
 *             return;
 *         }<p>
 *
 *         put(S_TAG_FORMAT, new String("ID3v2"));<p>
 *
 *         loadFrames(stream);<p>
 *
 *         buildKeyMapTable();<p>
 *
 *     }<p>
 *
 *     public String toString() {
 *         // Override default implementation if neccessary.
 *     }<p>
 *
 *     public Object clone() {
 *         // Override default implementation if neccessary.
 *     }<p>
 *
 *     //====================<p>
 *
 * }
 * </pre>
 * </blockquote>
 *
 * @author    Michael Scheerer
 */
public abstract class Tag extends Hashtable {
	
	/**
	 * The predefined tag version key.
	 */
	public final static String S_TAG_FORMAT = "String tagFormat";

	/**
	 * The predefined tag value key.
	 */
	public final static String B_TAG = "Boolean tag";
	
	/**
	 * The predefined album  value key.
	 */
	public final static String S_ALBUM = "String album";
	
	/**
	 * The predefined artist value key.
	 */
	public final static String S_ARTIST = "String artist";
	
	/**
	 * The predefined composer value key.
	 */
	public final static String S_COMPOSER = "String composer";
	
	/**
	 * The predefined track (number) value key.
	 */
	public final static String S_TRACK = "String track";
	
	/**
	 * The predefined year (of publishing) value key.
	 */
	public final static String S_YEAR = "String year";
	
	/**
	 * The predefined date (of publishing) value key.
	 */
	public final static String S_DATE = "String date";
	
	/**
	 * The predefined title value key.
	 */
	public final static String S_TITLE = "String title";
	
	/**
	 * The predefined performer value key.
	 */
	public final static String S_PERFORMER = "String performer";
	
	/**
	 * The predefined copyright text value key.
	 */
	public final static String S_COPYRIGHT = "String copyrightText";
	
	/**
	 * The predefined genre text value key.
	 */
	public final static String S_GENRE = "String genre";
		
	/**
	 * The predefined isrc (International Standard Recording Code) value key.
	 */
	public final static String S_ISRC = "String isrc";
	
	/**
	 * The predefined encoder value key.
	 */
	public final static String S_ENCODER = "String encoder";
	
	
	/**
	 * Creates a new instance. Tag information is completely read the first time it
	 * is requested and written after <code>update()</code>.
	 *
	 * @exception IOException   if I/O error occurs
	 */
	public Tag() throws IOException {}

	/**
	 * Frees all system resources, which are bounded to this object.
	 */
	public void close() {
		clear();
	}
	
	/**
	 * Returns a <code>Hashtable</code> representation of this <code>Information</code> object.
	 *
	 * @return     a <code>Hashtable</code> representation of this <code>Information</code> object
	 */
	public Hashtable getHashtable() {
		return (Hashtable) this.clone();
	}
}


