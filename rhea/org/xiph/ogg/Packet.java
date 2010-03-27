/* Ogg, created by Xiph,
 * ported from C++ to Java and named as Jorbis framework.
 *
 * Ogg decoder (c) 2000 Xiph foundation www.xiph.org
 * Jorbis framework (c) 2000 ymnk, JCraft,Inc. <ymnk@jcraft.com>
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


package org.xiph.ogg;

/**
 * The <code>Packet</code> class provides all necessary packet related members.
 *
 * @author	Xip, JCraft (Port)
 */
public final class Packet{
	public byte[] packetBase;
	public int packet;
	public int bytes;
	public int bos;
	public int eos;

	public long granulepos;

    /**
     * sequence number for decode; the framing
     * knows where there's a hole in the data,
     * but we need coupling so that the codec
     * (which is in a seperate abstraction
     * layer) also knows about the gap
     */
	public long packetno;
}
