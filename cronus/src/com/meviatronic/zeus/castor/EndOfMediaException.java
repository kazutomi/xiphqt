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
 * This <code>Exception</code> should be thrown if an end of media exception occurs.
 *
 * @author    Michael Scheerer
 */
public final class EndOfMediaException extends IOException {

	/**
	 * Constructs an instance of <code>EndOfMediaException</code>.
	 *
	 * @param s  the exception message
	 */
	public EndOfMediaException(java.lang.String s) {
		super(s);
	}
}

