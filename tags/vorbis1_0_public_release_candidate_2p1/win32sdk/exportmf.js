/*
 * $Id: exportmf.js,v 1.1 2001/10/20 18:08:22 cwolf Exp $
 *
 * Start msdev and export makefiles and dependency files.
 */

var msdev = WScript.CreateObject("msdev.application");
var shell = WScript.CreateObject("WScript.Shell");
var env = shell.Environment("PROCESS");
var SRCROOT = env("SRCROOT");

if (SRCROOT.length < 1)
{
  WScript.Echo("Error: " + WScript.ScriptFullName + " \"SRCROOT\" not set.");
  WScript.Quit(1);
}

msdev.Documents.Open(SRCROOT + "\\win32sdk\\all.dsw", "", true);
msdev.ActiveProject = msdev.Projects("all");
msdev.ExecuteCommand("BuildProjectExport");
msdev.Documents.CloseAll();
msdev.Quit();
