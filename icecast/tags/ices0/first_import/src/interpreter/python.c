/* python.c
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

static PyThreadState *mainthreadstate = NULL;
static PyObject *ices_python_module;
static PyThreadState *interpreter_python_init_thread ();
static void interpreter_python_shutdown_thread (PyThreadState *threadstate);
static void interpreter_python_setup_path ();

static void
interpreter_python_init ()
{
	interpreter_python_setup_path ();

	Py_Initialize ();
	PyEval_InitThreads ();
	
	mainthreadstate = PyThreadState_Get ();
	/* ices_python_init ();  This is if we want to export C functions to the python scripts */
	
	ices_log_debug ("Importing ices.py module...");

	if (!(ices_python_module = PyImport_ImportModule ("ices"))) {
		ices_log ("Error: Could not import module ices");
		PyErr_Print();
	} else {
		ices_log_debug ("Calling testfunction in ices python module");
		PyObject_CallMethod (ices_python_module, "testfunction", NULL);
	}

	PyEval_ReleaseLock ();
}

static void
interpreter_python_setup_path ()
{
	char *pythonpath = getenv ("PYTHONPATH");
	char newpath[1024];

	if (pythonpath && (strlen (pythonpath) > 900)) {
		ices_log ("Environment variable PYTHONPATH is too long, please rectify!");
		exit (0);
	}

	if (pythonpath) {
		sprintf (newpath, "%s:%s:.", pythonpath, ICES_MODULEDIR);
	} else {
		sprintf (newpath, "%s:.", ICES_MODULEDIR);
	}

	putenv (newpath);
}

static void
interpreter_python_shutdown ()
{
  PyEval_AcquireLock ();
  Py_Finalize ();
}

static PyThreadState *
interpreter_python_init_thread ()
{
  PyInterpreterState *maininterpreterstate = NULL;
  PyThreadState *newthreadstate;

  PyEval_AcquireLock ();

  maininterpreterstate = mainthreadstate->interp;
  newthreadstate = PyThreadState_New (maininterpreterstate);

  PyEval_ReleaseLock ();

  return newthreadstate;
}

static void
interpreter_python_shutdown_thread (PyThreadState *threadstate)
{
  PyEval_AcquireLock ();

  PyThreadState_Clear (threadstate);

  PyThreadState_Delete (threadstate);

  PyEval_ReleaseLock ();
}

void *
interpreter_python_eval_function (char *functionname)
{
	PyObject *ret;

	PyThreadState *threadstate = interpreter_python_init_thread ();
	
	PyEval_AcquireLock ();
	
	PyThreadState_Swap (threadstate);
	
	ices_python_module = PyImport_ReloadModule (ices_python_module);
	
	ices_log_debug ("Interpreting [%s]", functionname);
	
	ret = PyObject_CallMethod (ices_python_module, functionname, NULL);

	PyThreadState_Swap (NULL);
	
	PyEval_ReleaseLock ();
	
	ices_log_debug ("Done interpreting [%s]", functionname);
	
	interpreter_python_shutdown_thread (threadstate);

	return ret;
}






