#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include <stdio.h>
#include "resource.h"
#include "encthread.h"

HANDLE event = NULL;
int width = 120, height = 120;
RECT bar1, bar2;
int prog1 = 0, prog2 = 0;
int moving = 0;
POINT pt;
HINSTANCE hinst;
int frame = 0;
HBITMAP hbm[12], temp;
HDC offscreen;
HMENU menu;
int encoding_done = 0;
double file_complete;
int totalfiles;
int numfiles;
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int animate = 0;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
        static char szAppName[] = "oggdropWin";
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;

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

        hwnd = CreateWindow(szAppName, "oggdrop", WS_POPUP | WS_DLGFRAME, CW_USEDEFAULT, CW_USEDEFAULT,
		width, height, NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	SetTimer(hwnd, 1, 80, NULL);

	
	frame = 0;
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF01), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF02), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF03), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF04), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF05), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF06), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF07), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF08), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF09), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF10), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF11), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    hbm[frame++] = LoadImage(hinst, MAKEINTRESOURCE(IDB_TF12), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
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
	int state;
	double percomp;

	switch (message) {
	case WM_CREATE:
		menu = LoadMenu(hinst, MAKEINTRESOURCE(IDR_MENU1));
		if (menu == NULL) MessageBox(hwnd, "ACK!", "ACK!", 0);
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

		SetRect(&bar1, 0, height - 23, file_complete * width, height - 13);
		SetRect(&bar2, 0, height - 12, percomp * width, height - 2);

		FillRect(offscreen, &bar1, (HBRUSH)GetStockObject(BLACK_BRUSH));
		FillRect(offscreen, &bar2, (HBRUSH)GetStockObject(BLACK_BRUSH));

		BitBlt(hdc, 0, 0, width, height, offscreen, 0, 0, SRCCOPY);

		EndPaint(hwnd, &ps);
		return 0;

	case WM_TIMER:
		if (animate) {
			frame++;
			if (frame >= 12) frame = 0;
			GetClientRect(hwnd, &rect);
			InvalidateRect(hwnd, &rect, FALSE);
		} else {
			frame = 0;
			GetClientRect(hwnd, &rect);
			InvalidateRect(hwnd, &rect, FALSE);
		}
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
			point.x = LOWORD(lParam);
			point.y = HIWORD(lParam);
			ClientToScreen(hwnd, &point);
			SetWindowPos(hwnd, 0, point.x - start.x, point.y - start.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
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
		switch (LOWORD(wParam)) {
		case IDM_QUIT:
			encoding_done = 1;
			PostQuitMessage(0);
			break;

		case IDM_ONTOP:
			state = GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND);
			if ((state & MF_CHECKED) == MF_CHECKED) {
				CheckMenuItem(menu, LOWORD(wParam), MF_UNCHECKED);
				SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
			} else {
				CheckMenuItem(menu, LOWORD(wParam), MF_CHECKED);
				SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOMOVE);
			}
			break;
			
		case IDM_BITRATE128:
			CheckMenuItem(menu, IDM_BITRATE128, MF_CHECKED);
			CheckMenuItem(menu, IDM_BITRATE160, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE192, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE256, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE350, MF_UNCHECKED);
			encthread_setbitrate(128);
			break;

		case IDM_BITRATE160:
			CheckMenuItem(menu, IDM_BITRATE128, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE160, MF_CHECKED);
			CheckMenuItem(menu, IDM_BITRATE192, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE256, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE350, MF_UNCHECKED);
			encthread_setbitrate(160);
			break;

		case IDM_BITRATE192:
			CheckMenuItem(menu, IDM_BITRATE128, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE160, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE192, MF_CHECKED);
			CheckMenuItem(menu, IDM_BITRATE256, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE350, MF_UNCHECKED);
			encthread_setbitrate(192);
			break;
			
		case IDM_BITRATE256:
			CheckMenuItem(menu, IDM_BITRATE128, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE160, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE192, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE256, MF_CHECKED);
			CheckMenuItem(menu, IDM_BITRATE350, MF_UNCHECKED);
			encthread_setbitrate(256);
			break;

		case IDM_BITRATE350:
			CheckMenuItem(menu, IDM_BITRATE128, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE160, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE192, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE256, MF_UNCHECKED);
			CheckMenuItem(menu, IDM_BITRATE350, MF_CHECKED);
			encthread_setbitrate(350);
			break;
		}
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

