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

/* Function to initialize the python interpreter */
static void
interpreter_python_initialize ()
{
	char *module_name;

	/* For some reason, python refuses to look in the
	 * current directory for modules */
	interpreter_python_setup_path ();

	/* Initialize the python structure and thread stuff */
	Py_Initialize ();
	PyEval_InitThreads ();
	
	mainthreadstate = PyThreadState_Get ();

	/* If user specified a certain module to be loaded,
	 * then obey */
	if (ices_config.interpreter_file) {
		module_name = ices_config.interpreter_file;
	} else {
		module_name = "ices";
	}

	ices_log_debug ("Importing %s.py module...", module_name);

	/* Call the python api code to import the module */
	if (!(ices_python_module = PyImport_ImportModule (module_name))) {
		ices_log ("Error: Could not import module %s", module_name);
		PyErr_Print();
	} else {
		ices_log_debug ("Calling testfunction in %s python module", module_name);
		PyObject_CallMethod (ices_python_module, "testfunction", NULL);
	}
	
	PyEval_ReleaseLock ();
}

/* Force the python interpreter to look in our module path
 * and in the current directory for modules */
static void
interpreter_python_setup_path ()
{
	char *pythonpath = getenv ("PYTHONPATH");
	char newpath[1024];

	if (pythonpath && (strlen (pythonpath) > 900)) {
		ices_log ("Environment variable PYTHONPATH is too long, please rectify!");
		ices_setup_shutdown ();
		return;
	}

	if (pythonpath) {
		sprintf (newpath, "PYTHONPATH=%s:%s:.", pythonpath, ICES_MODULEDIR);
	} else {
		sprintf (newpath, "PYTHONPATH=%s:.", ICES_MODULEDIR);
	}

	putenv (newpath);
}

/* Shutdown the python interpreter */
static void
interpreter_python_shutdown ()
{
  PyEval_AcquireLock ();
  Py_Finalize ();
}

/* Startup a new python thread */
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

/* Shutdown the thread */
static void
interpreter_python_shutdown_thread (PyThreadState *threadstate)
{
  PyEval_AcquireLock ();

  PyThreadState_Clear (threadstate);

  PyThreadState_Delete (threadstate);

  PyEval_ReleaseLock ();
}

/* Evaluate the python function in a new thread */
void *
interpreter_python_eval_function (char *functionname)
{
	PyObject *ret;

	/* Create a new python thread */
	PyThreadState *threadstate = interpreter_python_init_thread ();
	
	PyEval_AcquireLock ();
	
	PyThreadState_Swap (threadstate);
	
	/* Reload the module (it might have changed) */
	ices_python_module = PyImport_ReloadModule (ices_python_module);
	
	ices_log_debug ("Interpreting [%s]", functionname);
	
	/* Call the python function */
	ret = PyObject_CallMethod (ices_python_module, functionname, NULL);

	PyThreadState_Swap (NULL);
	
	PyEval_ReleaseLock ();
	
	ices_log_debug ("Done interpreting [%s]", functionname);
	
	interpreter_python_shutdown_thread (threadstate);

	return ret;
}
