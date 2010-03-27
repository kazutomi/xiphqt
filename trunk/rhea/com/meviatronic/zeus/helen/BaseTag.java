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

/**
 * The <code>BaseTagContent</code> class contains the cloned information related to an Ogg Tag and
 * is designed to be as flexible as possible to
 * reduce the number of cases where information has to be returned as binary
 * when it is rather more structured. <p>
 *
 * It provides storage for - a type (e.g. a MIME-type or a language, Text) - a
 * subtype (text or binary) - a description (text) - the content (text or
 * binary).
 *
 * @author    Michael Scheerer
 */
public final class BaseTag extends Hashtable {
		
	/**
	 * Returns a <code>Hashtable</code> representation of this <code>Information</code> object.
	 *
	 * @return     a <code>Hashtable</code> representation of this <code>Information</code> object
	 */
	public Hashtable getHashtable() {
		return (Hashtable) this.clone();
	}
}


