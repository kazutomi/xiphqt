#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <stdio.h>
#include <commctrl.h>

#include "resource.h"
#include "encthread.h"
#include "audio.h"

#define LOSHORT(l)           ((SHORT)(l))
#define HISHORT(l)           ((SHORT)(((DWORD)(l) >> 16) & 0xFFFF))

#define BASEKEY "Software\\Xiphophorus\\Oggdrop"

HANDLE event = NULL;
int width = 120, height = 120;
RECT bar1, bar2;
int prog1 = 0, prog2 = 0;
int moving = 0;
POINT pt;
HINSTANCE hinst;
int frame = 0;
HBITMAP hbm[12], temp;
HMENU menu;
int encoding_done = 0;
double file_complete;
int totalfiles;
int numfiles;
HWND g_hwnd;
HWND qcwnd;
int qcValue;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int animate = 0;

BOOL CALLBACK QCProc(HWND hwndDlg, UINT message, 
                     WPARAM wParam, LPARAM lParam) ;

int get_base_key(HKEY* key)
{
	DWORD disposition;
	return ERROR_SUCCESS != RegCreateKeyEx(HKEY_CURRENT_USER, 
         BASEKEY, 0, 0, 0, KEY_ALL_ACCESS, 0, key, &disposition);
}

int read_setting(const char* name, int default_value)
{
	HKEY base_key;
	if (!get_base_key(&base_key))
	{
		int value;
		DWORD cb_value = sizeof(int);
		if (ERROR_SUCCESS == RegQueryValueEx(base_key, name, NULL, NULL, 
                                         (byte*)&value, &cb_value))
			return value;
	}
	return default_value;
}

void write_setting(const char* name, int value)
{
	HKEY base_key;
	if (!get_base_key(&base_key))
		RegSetValueEx(base_key, name, 0, REG_DWORD, (byte*)&value, sizeof(int));
}

void set_quality_coefficient(int v)
{
  encthread_setquality(v);
  write_setting("quality", v);
}

void set_always_on_top(HWND hwnd, int v)
{
	CheckMenuItem(menu, IDM_ONTOP, v ? MF_CHECKED : MF_UNCHECKED);
	SetWindowPos(hwnd, v ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
	write_setting("always_on_top", v);
}

void set_logerr(HWND hwnd, int v)
{
	CheckMenuItem(menu, IDM_LOGERR, v ? MF_CHECKED : MF_UNCHECKED);
	set_use_dialogs(v);
	write_setting("logerr", v);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static char szAppName[] = "oggdropWin";
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;
  const int width = 120;
	const int height = 120;
	int x;
	int y;

	hinst = hInstance;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON1));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;

	RegisterClass(&wndclass);

	x = max(min(read_setting("window_x", 64), GetSystemMetrics(SM_CXSCREEN) - width), 0);
  y = max(min(read_setting("window_y", 64), GetSystemMetrics(SM_CYSCREEN) - height), 0);

  hwnd = CreateWindow(szAppName, "OggDrop", WS_POPUP | WS_DLGFRAME, x, y,
  width, height, NULL, NULL, hInstance, NULL);

  g_hwnd = hwnd;

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	SetTimer(hwnd, 1, 80, NULL);

  qcValue = read_setting("quality", 75);
  set_quality_coefficient(qcValue);
	set_always_on_top(hwnd, read_setting("always_on_top", 1));
	set_logerr(hwnd, read_setting("logerr", 0));
	
	for (frame = 0; frame < 12; frame++)
		hbm[frame] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF01 + frame), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	frame = 0;

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	for (frame = 0; frame < 12; frame++) 
		DeleteObject(hbm[frame]);

	return msg.wParam;
}

void HandleDrag(HWND hwnd, HDROP hDrop)
{
	int cFiles, i;
	char szFile[MAX_PATH];
	char *ext;
	int flag = 0;

	cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
	for (i = 0; i < cFiles; i++) {
		DragQueryFile(hDrop, i, szFile, sizeof(szFile));

		if (ext = strrchr(szFile, '.')) {
			if (stricmp(ext, ".wav") == 0) {
				flag = 1;
				encthread_addfile(szFile);
			}
		}
	}

	DragFinish(hDrop);

	if (flag)
		SetEvent(event);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc, hmem;
	static HDC offscreen;
	PAINTSTRUCT ps;
	RECT rect;
	BITMAP bm;
	POINT point;
	static POINT start;
	static int dragging = 0;
	HDC desktop;
	HBITMAP hbitmap;
	HANDLE hdrop;
	double percomp;

	switch (message) {
	case WM_CREATE:
		menu = LoadMenu(hinst, MAKEINTRESOURCE(IDR_MENU1));
		menu = GetSubMenu(menu, 0);

		offscreen = CreateCompatibleDC(NULL);
		desktop = GetDC(GetDesktopWindow());
		hbitmap = CreateCompatibleBitmap(desktop, 200, 200);
		ReleaseDC(GetDesktopWindow(), desktop);
		SelectObject(offscreen, hbitmap);

		// Start the engines
		encthread_init();

		// We accept drag&drop
		DragAcceptFiles(hwnd, TRUE);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rect);
		width = rect.right + 1;
		height = rect.bottom + 1;

		FillRect(offscreen, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
		DrawText(offscreen, "Drop Files Here", -1, &rect, DT_SINGLELINE | DT_CENTER);

		hmem = CreateCompatibleDC(offscreen);
		SelectObject(hmem, hbm[frame]);
		GetObject(hbm[frame], sizeof(BITMAP), &bm);
		BitBlt(offscreen, width / 2 - 33, height / 2 - 33, bm.bmWidth, bm.bmHeight, hmem, 0, 0, SRCCOPY);
		DeleteDC(hmem);
		
		percomp = ((double)(totalfiles - numfiles) + 1 - (1 - file_complete)) / (double)totalfiles;

		SetRect(&bar1, 0, height - 23, (int)(file_complete * width), height - 13);
		SetRect(&bar2, 0, height - 12, (int)(percomp * width), height - 2);

		FillRect(offscreen, &bar1, (HBRUSH)GetStockObject(BLACK_BRUSH));
		FillRect(offscreen, &bar2, (HBRUSH)GetStockObject(BLACK_BRUSH));

		BitBlt(hdc, 0, 0, width, height, offscreen, 0, 0, SRCCOPY);

		EndPaint(hwnd, &ps);

    return DefWindowProc(hwnd, message, wParam, lParam);
		//return 0;

	case WM_TIMER:
		if (animate || frame) {
			frame--;
			if (frame < 0) 
				frame += 12;
		} else {
			frame = 0;
		}
		GetClientRect(hwnd, &rect);
		InvalidateRect(hwnd, &rect, FALSE);
		return 0;

	case WM_LBUTTONDOWN:
		start.x = LOWORD(lParam);
		start.y = HIWORD(lParam);
		ClientToScreen(hwnd, &start);
		GetWindowRect(hwnd, &rect);
		start.x -= rect.left;
		start.y -= rect.top;
		dragging = 1;
		SetCapture(hwnd);
		return 0;

	case WM_LBUTTONUP:
		if (dragging) {
			dragging = 0;
			ReleaseCapture();
		}
		return 0;

	case WM_MOUSEMOVE:
		if (dragging) {
      point.x = LOSHORT(lParam);
      point.y = HISHORT(lParam);
      /* lParam can contain negative coordinates !
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
      */
			ClientToScreen(hwnd, &point);
			SetWindowPos(hwnd, 0, point.x - start.x, point.y - start.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			write_setting("window_x", point.x - start.x);
			write_setting("window_y", point.y - start.y);
		}
		return 0;

	case WM_CAPTURECHANGED:
		if (dragging) {
			dragging = 0;
			ReleaseCapture();
		}
		return 0;

	case WM_RBUTTONUP:
		point.x = LOWORD(lParam);
		point.y = HIWORD(lParam);
		ClientToScreen(hwnd, &point);
		TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, hwnd, NULL);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) 
    {
 
		case IDM_QUIT:
			encoding_done = 1;
			PostQuitMessage(0);
			break;
		case IDM_ONTOP:
			set_always_on_top(hwnd, ~GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED);
			break;	
		case IDM_LOGERR:
			set_logerr(hwnd, ~GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED);
			break;

    case IDM_SAVEQUALITY:
      {
        int value = 
          DialogBox(
                hinst,  
                MAKEINTRESOURCE(IDD_QUALITY),   
                hwnd, QCProc);

        if (value != -1)
          set_quality_coefficient(value);
      }
      break;

    } // LOWORD(wParam)
		return 0;

	case WM_DROPFILES:
		hdrop = (HANDLE)wParam;
		HandleDrag(hwnd, hdrop);
		return 0;
		

	case WM_DESTROY:
		encoding_done = 1;
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

 /**
  * Saved quality coefficient slider dialog procedure.
  */
BOOL CALLBACK QCProc(HWND hwndDlg, UINT message, 
                     WPARAM wParam, LPARAM lParam) 
{
  switch (message) 
  { 
  case WM_INITDIALOG: 
 
    SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETRANGE, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) MAKELONG(0, 100));  // min. & max. positions 

    SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPAGESIZE, 
        0, (LPARAM) 4);                  // new page size 
 
    SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETSEL, 
        (WPARAM) FALSE,                  // redraw flag 
        (LPARAM) MAKELONG(0, 100));

    SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPOS, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) read_setting("quality", 75)); 
    break;

   case WM_HSCROLL:

    qcValue = (LONG)SendDlgItemMessage(hwndDlg, IDC_SLIDER1, 
                        TBM_GETPOS, (WPARAM)0, (LPARAM)0 );
    break;

    case WM_CLOSE:
      EndDialog(hwndDlg, -1);
    break;


    case WM_COMMAND: 
      switch (LOWORD(wParam)) 
      { 
        case IDC_BUTTON1:
        {
          EndDialog(hwndDlg, qcValue);
          return TRUE;
        }
        default: 
            break; 
      } 
  } 
  return FALSE; 
} 
 
