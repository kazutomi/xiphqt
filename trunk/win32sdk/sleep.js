/**
 * WSH implementation of "sleep" for windows, since 
 * windows batch does have support for "sleep".
 *
 * Parameters: number of seconds to sleep
 *
 * $Id: sleep.js,v 1.1 2001/10/18 03:28:24 cwolf Exp $
 */
var a;
WScript.Interactive = false;
argv = WScript.Arguments;

if (argv.length > 0)
  WScript.Sleep(argv(0) * 1000);
