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

#define BASEKEY "Software\\Xiph.Org\\Oggdrop"

#define VORBIS_DEFAULT_QUALITY 40

#define CREATEFONT(sz) \
  CreateFont((sz), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
              VARIABLE_PITCH | FF_SWISS, "")
   
HANDLE event = NULL;
int width = 130, height = 130;
RECT bar1, bar2, vbrBR;
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
HFONT font2;
int qcValue;
int bitRate;
char *fileName;
OE_MODE oe_mode;
float nominalBitrate;
int showNBR;
char approxBRCaption[80];


static const char *bitRateCaption[] =
{
  "64",
  "80",
  "96",
  "112",
  "128",
  "160",
  "192",
  "224",
  "256",
  "288",
  "320",
  "352",
  0
};

typedef union
{
	int buflen;
	char buf[16];
} EBUF;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
int animate = 0;

BOOL CALLBACK QCProc(HWND hwndDlg, UINT message, 
                     WPARAM wParam, LPARAM lParam) ;


float qc2approxBitrate(float qcValue, int rate, int channels);


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
                                         (BYTE*)&value, &cb_value))
		return value;
	}
	return default_value;
}

void write_setting(const char* name, int value)
{
	HKEY base_key;
	if (!get_base_key(&base_key))
		RegSetValueEx(base_key, name, 0, REG_DWORD, (BYTE*)&value, sizeof(int));
}

void set_quality_coefficient(int v)
{
	encthread_setquality(v);
	write_setting("quality", v);
}

void set_bitrate(int v)
{
	encthread_setbitrate(v);
	write_setting("bitrate", v);
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

void set_showNBR(HWND hwnd, int v)
{
	CheckMenuItem(menu, IDM_SHOWNBR, v ? MF_CHECKED : MF_UNCHECKED);
	write_setting("shownbr", v);
	showNBR = v;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
    static char szAppName[] = "oggdropWin";
	HWND hwnd;
	MSG msg;
	WNDCLASS wndclass;
	const int width = 130;
	const int height = 130;
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

	font2 = CREATEFONT(10);

	SetTimer(hwnd, 1, 80, NULL);

	qcValue = read_setting("quality", VORBIS_DEFAULT_QUALITY);
	set_quality_coefficient(qcValue);
	set_always_on_top(hwnd, read_setting("always_on_top", 1));
	set_logerr(hwnd, read_setting("logerr", 0));
	set_showNBR(hwnd, read_setting("shownbr", 1));
	(void) strcpy(approxBRCaption, "Nominal Bitrate");
	
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
	HFONT dfltFont;
	int dfltBGMode;
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

    	//SetRect(&vbrBR, 0, height - (height-14), width, height - (height-26));
	    SetRect(&vbrBR, 0, height - 35, width, height - 19);

    	dfltBGMode = SetBkMode(offscreen, TRANSPARENT);
	    dfltFont = SelectObject(offscreen, font2);

    	if (showNBR)
    	{
      		char nbrCaption[80];

			(void) sprintf(nbrCaption, "%s: %.1f kbit/s ", 
			approxBRCaption, nominalBitrate/1000);

			DrawText(offscreen, nbrCaption, -1, &vbrBR, DT_SINGLELINE | DT_CENTER);
		}


		SetRect(&bar1, 0, height - 23, (int)(file_complete * width), height - 13);
		SetRect(&bar2, 0, height - 12, (int)(percomp * width), height - 2);

		FillRect(offscreen, &bar1, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
		FillRect(offscreen, &bar2, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

		if (fileName)
		{
			char* sep;
			char  fileCaption[80];

			if ((sep = strrchr(fileName, '\\')) != 0)
				fileName = sep+1;

			(void) strcpy(fileCaption, "   ");
			(void) strcat(fileCaption, fileName);

			DrawText(offscreen, fileCaption, -1, &bar1, DT_SINGLELINE | DT_LEFT);
		}

		SelectObject(offscreen, dfltFont);
		SetBkMode(offscreen, dfltBGMode);

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
		case IDM_SHOWNBR:
			set_showNBR(hwnd, ~GetMenuState(menu, LOWORD(wParam), MF_BYCOMMAND) & MF_CHECKED);
			break;
		case IDM_SAVEQUALITY:
			{
				int value = 
				DialogBox(
                	hinst,  
                	MAKEINTRESOURCE(IDD_QUALITY),   
                	hwnd, QCProc);

				if (value == -2)
				set_bitrate(bitRate);
				else if (value != -1)
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

int sliding=FALSE;

 /**
  *  Encode parameters dialog procedures.
  */
BOOL CALLBACK QCProc(HWND hwndDlg, UINT message, 
                     WPARAM wParam, LPARAM lParam) 
{
	char editBuf[16];
	EBUF buf2;
	int br, i, len;

	switch (message) 
	{ 
	case WM_INITDIALOG: 
 
    	sliding = FALSE;

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
        	(LPARAM) read_setting("quality", VORBIS_DEFAULT_QUALITY));

    	SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETLIMITTEXT,
        	(WPARAM) 4, (LPARAM)0);

    	(void) sprintf(editBuf, "%02.1f", 
        (float) ((float)read_setting("quality", VORBIS_DEFAULT_QUALITY)/10.0));

    	SendDlgItemMessage(hwndDlg, IDC_EDIT1, WM_SETTEXT,(WPARAM)0, (LPARAM)editBuf);

    	SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));

    	SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL,(WPARAM)0, (LPARAM)-1);

		  qcValue = read_setting("quality", VORBIS_DEFAULT_QUALITY);

			nominalBitrate = qc2approxBitrate((float) qcValue, 44100, 2);
	    (void) sprintf(editBuf, "%03.1fkbps", nominalBitrate/1000);

    	SendDlgItemMessage(hwndDlg, IDC_EDIT2, WM_SETTEXT,(WPARAM)0, (LPARAM)editBuf);

    	(void) CheckRadioButton(hwndDlg, IDC_USEQUALITY,
        	                             IDC_USEBITRATE,
            	                         read_setting("mode", IDC_USEQUALITY));

    	for (br=0; bitRateCaption[br] != 0; br++)
    	{
      	SendDlgItemMessage(hwndDlg, IDC_BITRATE, CB_ADDSTRING,
          	(WPARAM)0, (LPARAM) bitRateCaption[br]);

      	SendDlgItemMessage(hwndDlg, IDC_BITRATE, CB_SETITEMDATA, 
          	(WPARAM) br, (LPARAM) atoi(bitRateCaption[br]));
    	}

    	bitRate = read_setting("bitrate", 128);
    	(void) sprintf(editBuf, "%d", bitRate);

   	for(br=0; bitRateCaption[br] != 0; br++)
    	{
      		if ( ! strcmp(bitRateCaption[br], editBuf) )
      		{
        		SendDlgItemMessage(hwndDlg, IDC_BITRATE, CB_SETCURSEL,
            		(WPARAM) br, (LPARAM) 0);
        		break;
      		}
    	}

   		break;

   case WM_HSCROLL:

    	sliding = TRUE;

    	qcValue = (LONG)SendDlgItemMessage(hwndDlg, IDC_SLIDER1, 
                        TBM_GETPOS, (WPARAM)0, (LPARAM)0 );

    	(void) sprintf(editBuf, "%02.1f", (float)(((float)qcValue/10.0)));

    	SendDlgItemMessage(hwndDlg, IDC_EDIT1, WM_SETTEXT,
        	(WPARAM)0, (LPARAM)editBuf);

    	SendDlgItemMessage(hwndDlg, IDC_EDIT1, WM_SETTEXT,(WPARAM)0, (LPARAM)editBuf);

//    	SetFocus(GetDlgItem(hwndDlg, IDC_EDIT1));
//    	SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL,(WPARAM)0, (LPARAM)-1);

			nominalBitrate = qc2approxBitrate((float) qcValue, 44100, 2);
			(void) sprintf(editBuf, "%03.1fkbps", nominalBitrate/1000);

			SendDlgItemMessage(hwndDlg, IDC_EDIT2, WM_SETTEXT,(WPARAM)0, (LPARAM)editBuf);


    	(void) CheckRadioButton(hwndDlg, IDC_USEQUALITY,
        	                         IDC_USEBITRATE,
            	                         IDC_USEQUALITY);
    	(void) strcpy(approxBRCaption, "Nominal Bitrate");
    
    	break;

    case WM_CLOSE:
      	EndDialog(hwndDlg, -1);
    	break;

    case WM_COMMAND: 
      	switch (LOWORD(wParam)) 
      	{ 
        	case IDC_BUTTON1:
          		if (IsDlgButtonChecked(hwndDlg, IDC_USEQUALITY) == BST_CHECKED)
          		{
            		write_setting("mode", IDC_USEQUALITY);
            		oe_mode = OE_MODE_QUALITY;
            		(void) strcpy(approxBRCaption, "Nominal Bitrate");
            		EndDialog(hwndDlg, qcValue);
          		}
          		else
          		{
            		write_setting("mode", IDC_USEBITRATE);
            		oe_mode = OE_MODE_BITRATE;
            		nominalBitrate = (float)(bitRate*1000);
            		(void) strcpy(approxBRCaption, "Bitrate");
            		EndDialog(hwndDlg, -2); // use bitrate
          		}
          		return TRUE;
        
        	case IDC_BITRATE:
          		(void) CheckRadioButton(hwndDlg, IDC_USEQUALITY,
                	                           	 IDC_USEBITRATE,
                                           		 IDC_USEBITRATE);

          		if ((br = SendDlgItemMessage(hwndDlg, IDC_BITRATE, CB_GETCURSEL,
                		   (WPARAM) 0, (LPARAM) 0)) != LB_ERR)
          		{
            			bitRate = SendDlgItemMessage(hwndDlg, IDC_BITRATE, CB_GETITEMDATA,
                   			(WPARAM) br, (LPARAM) 0);
          		}

          		(void) strcpy(approxBRCaption, "Bitrate");
          		nominalBitrate = (float)(bitRate*1000);

          		break;

        	case IDC_EDIT1:
 
          		switch (HIWORD(wParam))
          		{
           
          			case EN_UPDATE:
          			{

            			buf2.buflen = sizeof(buf2.buf);

            			len = SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_GETLINE,
              					(WPARAM) 0, (LPARAM) buf2.buf);

            			buf2.buf[len] = '\0';

            			for (i=0; i<len; i++)
            			{
              				if ( ! isdigit(buf2.buf[i]) && buf2.buf[i] != '.')
              				{
                				buf2.buf[i] = '\0';
                				SendDlgItemMessage(hwndDlg, IDC_EDIT1, WM_SETTEXT,
                  					(WPARAM) 0, (LPARAM) buf2.buf);
              				}
            			}
            
            			if ( atof(buf2.buf) > 10.0f )
            			{
              				SendDlgItemMessage(hwndDlg, IDC_EDIT1, WM_SETTEXT,
                  				(WPARAM) 0, (LPARAM) "10.0");

              				SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_SETSEL,
                  				(WPARAM)0, (LPARAM)-1);
            			}

            			(void) CheckRadioButton(hwndDlg, IDC_USEQUALITY,
                        			                     IDC_USEBITRATE,
                                    			         IDC_USEQUALITY);
            			(void) strcpy(approxBRCaption, "Nominal Bitrate");
          			}
            
            		break;

          			case EN_CHANGE:
            			if ( ! sliding )
            			{
              				float v=0.0;

              				buf2.buflen = sizeof(buf2.buf);
              				len = SendDlgItemMessage(hwndDlg, IDC_EDIT1, EM_GETLINE,
              						(WPARAM) 0, (LPARAM) buf2.buf);

              				buf2.buf[len] = '\0';
            
              				v = (float)atof(buf2.buf);

              				qcValue = (int)(v*10.0f);

              				//MessageBox(g_hwnd, buf2.buf, "Second", 0);

              				SendDlgItemMessage(hwndDlg, IDC_SLIDER1, TBM_SETPOS, 
                					(WPARAM) TRUE,                   // redraw flag 
                					(LPARAM)(int)(v*10.0f));
            			}
            			else
              				sliding = FALSE;

            
          			default:
            		break;
          		}

        		default: 
            	break; 
      		}
      		break; 


      	default:
        break;
	} 
	return FALSE; 
} 


float qc2approxBitrate(float qcValue, int rate, int channels)
{
#if 1  /* this is probably slower, but always portable/accurate. */
    vorbis_info vi;
    float br;

    vorbis_info_init(&vi);
    if(vorbis_encode_init_vbr(&vi, channels, rate, (float) (qcValue / 100.0))) 
        return 128000.0; /* Mode setup failed: go with a default. */

    br = (float)vi.bitrate_nominal;
    vorbis_info_clear(&vi);

    return br;

#else   /* this is fast, but it wings it with incorrect results. */

  float approxBitrate;
  double scale;

  if ( qcValue < 41)
  {
    approxBitrate = 1000 * (float)(((float)qcValue/10.0)*32);
  }
  else if ( qcValue < 81 )
  {
    approxBitrate = 1000 * (float)(((float)qcValue/10.0)*32);
  }
  else if ( qcValue < 91 )
  {
    approxBitrate = 1000 *(float)((((float)qcValue/10.0)*32)+
							       ((((float)qcValue/10)-8)*32));
  }
  else
  {
    approxBitrate = 1000 * (float)((((float)qcValue/10.0)*32)+
	    ((((float)qcValue/10)-8)*32)+((((float)qcValue/10)-9)*116));
  }

  if (approxBitrate < 64000.0)
    approxBitrate = 64000.0;

  return approxBitrate;
#endif
}
