/* playlist_python.c
 * - Interpreter functions for python
 * Copyright (c) 2000 Alexander Haväng
 * Copyright (c) 2001 Brendan Cully
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

#include "definitions.h"
#ifdef _REENTRANT
#undef _REENTRANT
#endif
#include <Python.h>

#define PM_PYTHON_MAKE_THREADS 0

extern ices_config_t ices_config;

static PyObject *python_module;

static char* pl_init_hook;
static char* pl_shutdown_hook;
static char* pl_get_next_hook;
static char* pl_get_metadata_hook;
static char* pl_get_lineno_hook;

#if PM_PYTHON_MAKE_THREADS
static PyThreadState *mainthreadstate = NULL;
#endif

/* -- local prototypes -- */
static int playlist_python_get_lineno (void);
static char* playlist_python_get_next (void);
static char* playlist_python_get_metadata (void);
static void playlist_python_shutdown (void);

static int python_init (void);
static void python_shutdown (void);
static void python_setup_path (void);
static PyObject* python_eval (char *functionname);
#if PM_PYTHON_MAKE_THREADS
static PyThreadState* python_init_thread (void);
static void python_shutdown_thread (PyThreadState *threadstate);
#endif
static char* python_find_attr (PyObject* module, char* f1, char* f2);

/* Call python function to inialize the python script */
int
ices_playlist_python_initialize (playlist_module_t* pm)
{
  PyObject* res;
  int rc = 1;

  pm->get_next = playlist_python_get_next;
  pm->get_metadata = playlist_python_get_metadata;
  pm->get_lineno = playlist_python_get_lineno;
  pm->shutdown = playlist_python_shutdown;

  if (python_init () < 0)
    return -1;

  if (pl_init_hook) {
    if ((res = python_eval (pl_init_hook)) && PyInt_Check (res))
      rc = PyInt_AsLong (res);
    else
      ices_log_error ("ices_init failed");
    
    Py_XDECREF (res);
  }
  
  return rc;
}

/* Call the python function to get the current line number */
static int
playlist_python_get_lineno (void)
{
  PyObject* res;
  int rc = 0;

  if (pl_get_lineno_hook) {
    if ((res = python_eval (pl_get_lineno_hook)) && PyInt_Check (res))
      rc = PyInt_AsLong (res);
    else
      ices_log_error ("ices_get_lineno failed");

    Py_XDECREF (res);
  }

  return rc;
}

/* Call python function to get next file to play */
static char *
playlist_python_get_next (void)
{
  PyObject* res;
  char* rc = NULL;

  if ((res = python_eval (pl_get_next_hook)) && PyString_Check (res))
    rc = ices_util_strdup (PyString_AsString (res));
  else
    ices_log_error ("ices_get_next failed");

  Py_XDECREF (res);

  return rc;
}

static char*
playlist_python_get_metadata (void)
{
  PyObject* res;
  char* rc = NULL;

  if (pl_get_metadata_hook) {
    if ((res = python_eval (pl_get_metadata_hook)) && PyString_Check (res))
      rc = ices_util_strdup (PyString_AsString (res));
    else
      ices_log_error ("ices_get_metadata failed");

    Py_XDECREF (res);
  }

  return rc;
}

/* Call python function to shutdown the script */
static void
playlist_python_shutdown (void)
{
  PyObject* res;

  if (pl_shutdown_hook) {
    if (! ((res = python_eval (pl_shutdown_hook)) && PyInt_Check (res)))
      ices_log_error ("ices_shutdown failed");

    Py_XDECREF (res);
  }

  python_shutdown ();
}

/* -- Python interpreter management -- */

/* Function to initialize the python interpreter */
static int
python_init (void)
{
  /* For some reason, python refuses to look in the
   * current directory for modules */
  python_setup_path ();

  /* Initialize the python structure and thread stuff */
  Py_Initialize ();
  PyEval_InitThreads ();
#if PM_PYTHON_MAKE_THREADS
  mainthreadstate = PyThreadState_Get ();
#endif

  ices_log_debug ("Importing %s.py module...", ices_config.pm.module);

  /* Call the python api code to import the module */
  if (!(python_module = PyImport_ImportModule (ices_config.pm.module))) {
    ices_log ("Error: Could not import module %s", ices_config.pm.module);
    PyErr_Print();
  }

  /* Find defined methods */
  pl_init_hook = python_find_attr (python_module, "ices_init",
				   "ices_python_initialize");
  pl_shutdown_hook = python_find_attr (python_module, "ices_shutdown",
				       "ices_python_shutdown");
  pl_get_next_hook = python_find_attr (python_module, "ices_get_next",
				       "ices_python_get_next");
  pl_get_metadata_hook = python_find_attr (python_module, "ices_get_metadata",
					   "ices_python_get_metadata");
  pl_get_lineno_hook = python_find_attr (python_module, "ices_get_lineno",
					 "ices_python_get_current_lineno");

  PyEval_ReleaseLock ();

  if (! pl_get_next_hook) {
    ices_log_error ("The playlist module must define at least the ices_get_next method");
    return -1;
  }

  return 0;
}

/* Force the python interpreter to look in our module path
 * and in the current directory for modules */
static void
python_setup_path (void)
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
python_shutdown (void)
{
  PyEval_AcquireLock ();
  Py_Finalize ();
}

/* Evaluate the python function in a new thread */
static PyObject*
python_eval (char *functionname)
{
  PyObject *ret;
#if PM_PYTHON_MAKE_THREADS
  /* Create a new python thread */
  PyThreadState *threadstate = python_init_thread ();
#endif
  PyEval_AcquireLock ();
#if PM_PYTHON_MAKE_THREADS
  PyThreadState_Swap (threadstate);
#endif

  ices_log_debug ("Interpreting [%s]", functionname);
	
  /* Call the python function */
  ret = PyObject_CallMethod (python_module, functionname, NULL);
  if (! ret)
    PyErr_Print ();
#if PM_PYTHON_MAKE_THREADS
  PyThreadState_Swap (mainthreadstate);
#endif
  PyEval_ReleaseLock ();

  ices_log_debug ("Done interpreting [%s]", functionname);
#if PM_PYTHON_MAKE_THREADS
  python_shutdown_thread (threadstate);
#endif
  return ret;
}

#if PM_PYTHON_MAKE_THREADS
/* Startup a new python thread */
static PyThreadState *
python_init_thread (void)
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
python_shutdown_thread (PyThreadState *threadstate)
{
  PyEval_AcquireLock ();

  PyThreadState_Clear (threadstate);

  PyThreadState_Delete (threadstate);

  PyEval_ReleaseLock ();
}
#endif

static char*
python_find_attr (PyObject* module, char* f1, char* f2)
{
  char* rc;

  if (PyObject_HasAttrString (module, f1))
    rc = f1;
  else if (PyObject_HasAttrString (module, f2))
    rc = f2;
  else
    rc = NULL;

  if (rc)
    ices_log_debug ("Found method: %s", rc);

  return rc;
}
