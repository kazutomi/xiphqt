/* playlist_python.c
 * - Interpreter functions for python
 * Copyright (c) 2000 Alexander Haväng
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <Python.h>

int
interpreter_playlist_python_get_current_lineno ()
{
	PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_get_current_lineno");

	if (res && PyInt_Check(res))
		return PyInt_AsLong (res);

	ices_log_error ("Execution of 'ices_python_get_current_lineno()' in ices.py failed");
	return 0;
}

char *
interpreter_playlist_python_get_next ()
{
	PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_get_next");

	if (res && PyString_Check(res))
		return ices_util_strdup (PyString_AsString (res));
	ices_log_error ("Execution of 'ices_python_get_next()' in ices.py failed");
	return NULL;
}

int
interpreter_playlist_python_initialize (ices_config_t *ices_config)
{
	PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_initialize");

	if (res && PyInt_Check(res))
		return PyInt_AsLong (res);

	ices_log_error ("Execution of 'ices_python_initialize()' in ices.py failed");
	return 0;
}


int
interpreter_playlist_python_shutdown (ices_config_t *ices_config)
{
	PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_shutdown");

	if (res && PyInt_Check(res))
		return PyInt_AsLong (res);

	ices_log_error ("Execution of 'ices_python_shutdown()' in ices.py failed");
	return 0;
}

