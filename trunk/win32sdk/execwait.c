/*
 * $Id: execwait.c,v 1.1 2001/10/18 17:20:30 cwolf Exp $
 */
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 1000

/**
 * Execute a command and wait for it's completion.
 * 
 * @author Matthijs Laan
 */
int main(int argc, char **argv)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  char cmdline[BUF_SIZE+1] = "";
  int i, c=0;

  if(argc==1)
  {
    (void)fprintf(stderr, "Usage: execandwait commands\n");
    exit(1);
  }

  for(i=1;i<argc;i++)
  {
    if(strlen(argv[i])>(size_t)(BUF_SIZE-c))
    {
      (void)fprintf(stderr, "Command line too long\n");
      exit(1);
    }

    strcat(cmdline, argv[i]);
    strcat(cmdline, " ");
    c+=strlen(argv[i]);
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
