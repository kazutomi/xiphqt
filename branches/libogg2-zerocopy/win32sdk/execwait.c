/*
 * $Id: execwait.c,v 1.2 2001/10/21 20:32:03 cwolf Exp $
 */
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <string.h>


/**
 * Execute a command and wait for it's completion.
 * 
 * @author Matthijs Laan
 */
int main(int argc, char **argv)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  char *cmdline;
  int   argend;
  int   i;

  if(argc==1)
  {
    (void)fprintf(stderr, "Usage: execandwait commands\n");
    exit(1);
  }

  for(i=1, 
      cmdline = (char *)malloc(strlen(argv[1]) + 1),
      *cmdline = '\0';
      i<argc; i++)
  {
    (void)strcat(cmdline, argv[i]);
    (void)strcat(cmdline, " ");

    argend = strlen(cmdline);

    if (i+1 < argc)
    {
      cmdline = (char *)realloc(cmdline, 
                                strlen(cmdline) + strlen(argv[i+1]) + 1);
      *(cmdline + argend) = '\0'; // restore null terminator
    }
  }

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);

  if(!CreateProcess(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
  {
    (void)fprintf(stderr, "CreateProcess failed\n");
    exit(1);
  }

  WaitForSingleObject(pi.hProcess, INFINITE);

  return 0;
}
