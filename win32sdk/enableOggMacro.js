/**
 * $Id: enableOggMacro.js,v 1.1 2001/10/18 03:29:06 cwolf Exp $
 *
 * Enable Ogg/Vorbis Macro
 *
 * This script will invoke MSDev and navigate through the menus
 * to perform the task of enabling the installed oggvorbis.dsm
 * macro file.
 *
 * This has been tested under MSVC6.
 */
var shell = WScript.CreateObject("WScript.Shell");
var env = shell.Environment("PROCESS");
var msdevDir = env("MSDEVDIR");
var keyDelay = 50;

shell.Run("msdev", true);

WScript.Sleep(keyDelay);
shell.AppActivate("Microsoft Visual C++");
WScript.Sleep(keyDelay);
shell.SendKeys("%TC");
WScript.Sleep(keyDelay);
shell.SendKeys("{TAB}");
WScript.Sleep(keyDelay);
shell.SendKeys("{TAB}");
WScript.Sleep(keyDelay);
shell.SendKeys("{TAB}");
WScript.Sleep(keyDelay);
shell.SendKeys("{TAB}");
WScript.Sleep(keyDelay);
shell.SendKeys("A");
WScript.Sleep(keyDelay);
shell.SendKeys("%B");
WScript.Sleep(keyDelay);
shell.SendKeys(msdevDir + "\\Macros\\oggvorbis.dsm");
WScript.Sleep(keyDelay);
shell.SendKeys("~{TAB}~");
WScript.Sleep(keyDelay);
shell.SendKeys("%Fx");
