/**
 * WSH implementation of "sleep" for windows, since 
 * windows batch does have support for "sleep".
 *
 * Parameters: number of seconds to sleep
 *
 * $Id: sleep.js,v 1.2 2001/10/18 03:44:19 cwolf Exp $
 */
WScript.Interactive = false;
argv = WScript.Arguments;

if (argv.length > 0)
  WScript.Sleep(argv(0) * 1000);
