/*
 * $Id: exportmf.js,v 1.1 2001/10/20 18:08:48 cwolf Exp $
 *
 * Start msdev and export makefiles and dependency files.
 */

var msdev = WScript.CreateObject("msdev.application");
var shell = WScript.CreateObject("WScript.Shell");
var env = shell.Environment("PROCESS");
var SDKHOME= env("SDKHOME");

if (SDKHOME.length < 1)
{
  WScript.Echo("Error: " + WScript.ScriptFullName + " \"SDKHOME\" not set.");
  WScript.Quit(1);
}

msdev.Documents.Open(SDKHOME + "\\build\\examples.dsw");
msdev.ActiveProject = msdev.Projects("examples");
msdev.ExecuteCommand("BuildProjectExport");
msdev.Documents.CloseAll();
msdev.Quit();
