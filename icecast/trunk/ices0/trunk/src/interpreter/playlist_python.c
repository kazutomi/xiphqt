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

/* -- local prototypes -- */
static int playlist_python_get_lineno (void);
static char* playlist_python_get_next (void);
static char* playlist_python_get_metadata (void);
static void playlist_python_shutdown (void);

/* Call python function to inialize the python script */
int
interpreter_playlist_python_initialize (playlist_module_t* pm)
{
  PyObject* res = (PyObject*) interpreter_python_eval_function ("ices_python_initialize");

  pm->get_next = playlist_python_get_next;
  pm->get_metadata = playlist_python_get_metadata;
  pm->get_lineno = playlist_python_get_lineno;
  pm->shutdown = playlist_python_shutdown;

  if (res && PyInt_Check (res))
    return PyInt_AsLong (res);

  ices_log_error ("ices_python_initialize failed");
  return 0;
}

/* Call the python function to get the current line number */
static int
playlist_python_get_lineno (void)
{
  PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_get_current_lineno");

  if (res && PyInt_Check (res))
    return PyInt_AsLong (res);

  ices_log_error ("ices_python_get_current_lineno failed");
  return 0;
}

/* Call python function to get next file to play */
static char *
playlist_python_get_next (void)
{
  PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_get_next");

  if (res && PyString_Check (res))
    return ices_util_strdup (PyString_AsString (res));
  ices_log_error ("ices_python_get_next failed");

  return NULL;
}

static char*
playlist_python_get_metadata (void)
{
  PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_get_metadata");

  if (res && PyString_Check (res))
    return ices_util_strdup (PyString_AsString (res));

  ices_log_error ("ices_python_get_metadata failed");
  return NULL;
}

/* Call python function to shutdown the script */
static void
playlist_python_shutdown (void)
{
  PyObject *res = (PyObject *)interpreter_python_eval_function ("ices_python_shutdown");

  if (res && PyInt_Check (res))
    return;

  ices_log_error ("ices_python_shutdown failed");
}

