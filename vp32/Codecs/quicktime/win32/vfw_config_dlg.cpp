//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//--------------------------------------------------------------------------


/****************************************************************************
*
*   Module Title :     vfw_config_dlg.c
*
*   Description  :     Configuration Parameters dialog module.
*
*/						

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

//#define STRICT              /* Strict type checking. */
#include <windows.h>
#include <stdio.h>  

//#define INC_WIN_HEADER


#include <commctrl.h>

#include "type_aliases.h"
#include "vfw_comp_interface.h"
#include "resource.h"		// Must be the version resident in the pre-processor dll directory!!!


extern void 
getCompConfigDefaultSettings(COMP_CONFIG *CompConfig);

/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

namespace 
{
    const int MIN_SENSITIVITY = 50;
    const int MAX_SENSITIVITY = 100;

    const int MIN_MINIMUM = 0;
    const int MAX_MINIMUM = 30;

    const int MIN_MAXIMUM = 0;
    const int MAX_MAXIMUM = 600;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
InitTrackbar( 
        HWND hwndTrack,  // handle of trackbar
        UINT iMin,     // minimum value in trackbar range 
        UINT iMax,     // maximum value in trackbar range 
        UINT pos)  // maximum value in trackbar selection 
{ 
    SendMessage(hwndTrack, TBM_SETRANGE, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) MAKELONG(iMin, iMax));  // min. & max. positions 

    SendMessage(hwndTrack, TBM_SETPOS, 
        (WPARAM) TRUE,                   // redraw flag 
        (LPARAM) pos); 
} 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void InitControls(
                HWND hWndDlg,
                COMP_CONFIG *CompConfig)
{
    //  Set check boxes.
    CheckDlgButton( hWndDlg, IDC_ALLOW_DROPPED_FRAMES_CHECK, (CompConfig->AllowDF) ? 1 : 0 );
    CheckDlgButton( hWndDlg, IDC_QUICK_COMPRESS_CHECK, (CompConfig->QuickCompress) ? 1 : 0 );
    CheckDlgButton( hWndDlg, IDC_AUTOKEYFRAME_CHECK, (CompConfig->AutoKeyFrameEnabled) ? 1 : 0 );

    /* Key Frame Difference Threshold */          
    SendMessage(GetDlgItem(hWndDlg, IDC_EDIT_DIFF), EM_LIMITTEXT, (WPARAM) 3, (LPARAM) 0); 
    SetDlgItemInt( hWndDlg, IDC_EDIT_DIFF, CompConfig->AutoKeyFrameThreshold, FALSE );

    InitTrackbar( GetDlgItem(hWndDlg, IDC_SLIDER_DIFF),
                    MIN_SENSITIVITY,     // minimum value in trackbar range 
                    MAX_SENSITIVITY,     // maximum value in trackbar range 
                    CompConfig->AutoKeyFrameThreshold);  // current position 

    /* Minimum # of frames in between keyframes */          
    SendMessage(GetDlgItem(hWndDlg, IDC_EDIT_MIN), EM_LIMITTEXT, (WPARAM) 2, (LPARAM) 0); 
    SetDlgItemInt( hWndDlg, IDC_EDIT_MIN, CompConfig->MinimumDistanceToKeyFrame, FALSE );

    InitTrackbar( GetDlgItem(hWndDlg, IDC_SLIDER_MIN),
                    MIN_MINIMUM,     // minimum value in trackbar range 
                    MAX_MINIMUM,     // maximum value in trackbar range 
                    CompConfig->MinimumDistanceToKeyFrame);  // current position 

    /* MAXIMUM # of frames in between keyframes */          
    SendMessage(GetDlgItem(hWndDlg, IDC_EDIT_MAX), EM_LIMITTEXT, (WPARAM) 3, (LPARAM) 0); 
    SetDlgItemInt( hWndDlg, IDC_EDIT_MAX, CompConfig->ForceKeyFrameEvery, FALSE );

    InitTrackbar( GetDlgItem(hWndDlg, IDC_SLIDER_MAX),
                    MIN_MAXIMUM,     // minimum value in trackbar range 
                    MAX_MAXIMUM,     // maximum value in trackbar range 
                    CompConfig->ForceKeyFrameEvery);  // current position 

    //enable/disable auto key frame related controls
    EnableWindow(GetDlgItem(hWndDlg, IDC_EDIT_DIFF), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_EDIT_MIN), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_EDIT_MAX), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_SLIDER_DIFF), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_SLIDER_MIN), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_SLIDER_MAX), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_STATIC_DIFF), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_STATIC_MIN), CompConfig->AutoKeyFrameEnabled);
    EnableWindow(GetDlgItem(hWndDlg, IDC_STATIC_MAX), CompConfig->AutoKeyFrameEnabled);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
BOOL FAR PASCAL Config_ParamsDlgProc(   HWND   hWndDlg,
                                        UINT   Message,
                                        WPARAM wParam,
                                        LPARAM lParam)
{
    COMP_CONFIG *CompConfig = 0;
    HWND hwndTrack = NULL;  // handle of trackbar
    int editID = 0;
     
     /* Variables to note values on input in case the user 
     *  wants to cancel. 
     */
	 static BOOL	TempAllowDroppedFrames = FALSE;	
	 static BOOL	TempQuickCompress = TRUE;	
	 static BOOL    TempAutoKeyFrameEnabled;
	 static INT32   TempAutoKeyFrameThreshold;
	 static UINT32  TempMinimumDistanceToKeyFrame;
	 static INT32   TempForceKeyFrameEvery;

    switch(Message)
    {        
        case WM_INITDIALOG:
            SetWindowLong(hWndDlg, GWL_USERDATA, (unsigned long)lParam);  
            CompConfig = (COMP_CONFIG *) lParam;

            //save values on entry
            TempAllowDroppedFrames = CompConfig->AllowDF;	
            TempQuickCompress = CompConfig->QuickCompress;
            TempAutoKeyFrameEnabled		= CompConfig->AutoKeyFrameEnabled;
            TempAutoKeyFrameThreshold		= CompConfig->AutoKeyFrameThreshold;
            TempMinimumDistanceToKeyFrame = CompConfig->MinimumDistanceToKeyFrame;
            TempForceKeyFrameEvery		= CompConfig->ForceKeyFrameEvery;

            InitControls(hWndDlg, CompConfig);

            return (TRUE);

        case WM_HSCROLL:
            CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);
            int *valueToChange;

            if ( ( HWND ) lParam == (hwndTrack = GetDlgItem(hWndDlg, IDC_SLIDER_DIFF)) )
            {
                editID = IDC_EDIT_DIFF;
                valueToChange = (int *)&CompConfig->AutoKeyFrameThreshold;
            }
            else if ( ( HWND ) lParam == (hwndTrack = GetDlgItem(hWndDlg, IDC_SLIDER_MIN)) )
            {
                editID = IDC_EDIT_MIN;
                valueToChange = (int *)&CompConfig->MinimumDistanceToKeyFrame;
            }
            else if ( ( HWND ) lParam == (hwndTrack = GetDlgItem(hWndDlg, IDC_SLIDER_MAX)) )
            {
                editID = IDC_EDIT_MAX;
                valueToChange = (int *)&CompConfig->ForceKeyFrameEvery;
            }


            switch(LOWORD(wParam))
            {
                case TB_LINEUP:
                case TB_LINEDOWN:
                case TB_PAGEUP:
                case TB_PAGEDOWN:
                case TB_THUMBPOSITION:
                case TB_TOP:
                case TB_BOTTOM:
                case TB_ENDTRACK:
                case TB_THUMBTRACK:
                    *valueToChange = SendMessage(hwndTrack, TBM_GETPOS, 0, 0); 
                    SetDlgItemInt( hWndDlg, editID, *valueToChange, FALSE );

                    //extra logic to make sure the min slider is never greater than the max slider
                    if ( ( HWND ) lParam == (GetDlgItem(hWndDlg, IDC_SLIDER_MIN)) )
                    {
                        if(*valueToChange > CompConfig->ForceKeyFrameEvery)
                        {
                            CompConfig->ForceKeyFrameEvery = *valueToChange;
                            SetDlgItemInt( hWndDlg, IDC_EDIT_MAX, CompConfig->ForceKeyFrameEvery, FALSE );

                            SendMessage(GetDlgItem(hWndDlg, IDC_SLIDER_MAX), TBM_SETPOS, (WPARAM) TRUE, 
                                            (LPARAM) *valueToChange);
                        }
                    }
                    else if ( ( HWND ) lParam == (GetDlgItem(hWndDlg, IDC_SLIDER_MAX)) )
                    {
                        if(*valueToChange < (int)CompConfig->MinimumDistanceToKeyFrame)
                        {
                            CompConfig->MinimumDistanceToKeyFrame = *valueToChange;
                            SetDlgItemInt( hWndDlg, IDC_EDIT_MIN, CompConfig->MinimumDistanceToKeyFrame, FALSE );

                            SendMessage(GetDlgItem(hWndDlg, IDC_SLIDER_MIN), TBM_SETPOS, (WPARAM) TRUE, 
                                            (LPARAM) *valueToChange);
                        }
                    }

                    break;

                default:
                    break;
            }        
            return (TRUE);

        case WM_CLOSE:          /* Close the dialog. */
            /* Closing the Dialog behaves the same as Cancel    */
            PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0L);
            return (TRUE);

        case WM_COMMAND:       /* A control has been activated. */
            switch(LOWORD(wParam))
            {
                /* OK leaves the current settings in force */
                case IDOK:
                    EndDialog(hWndDlg, IDOK);
                    break; 

                case IDCANCEL:
                    CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);

                    /* Restore values on entry. */
                    CompConfig->AllowDF = TempAllowDroppedFrames;	
                    CompConfig->QuickCompress = TempQuickCompress;
                    CompConfig->AutoKeyFrameEnabled			= TempAutoKeyFrameEnabled;
                    CompConfig->AutoKeyFrameThreshold		= TempAutoKeyFrameThreshold;
                    CompConfig->MinimumDistanceToKeyFrame	= TempMinimumDistanceToKeyFrame;
                    CompConfig->ForceKeyFrameEvery			= TempForceKeyFrameEvery;

                    EndDialog(hWndDlg, IDCANCEL);
                    break;

                case IDDEFAULT:
                    CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);
        
                    getCompConfigDefaultSettings(CompConfig);
        
                    InitControls(hWndDlg, CompConfig);
                    break;

                case IDC_ALLOW_DROPPED_FRAMES_CHECK:
                    CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);

                    if( HIWORD( wParam ) == BN_CLICKED )
                    {
                      switch (LOWORD(wParam))
                      {
	                      case IDC_ALLOW_DROPPED_FRAMES_CHECK:
		                      CompConfig->AllowDF = (CompConfig->AllowDF) ? 0 : 1;
  		                      CheckDlgButton( hWndDlg, IDC_ALLOW_DROPPED_FRAMES_CHECK, (CompConfig->AllowDF) ? 1 : 0 );
		                      break;
                      }
                    }
                    break;

                case IDC_QUICK_COMPRESS_CHECK:
                    CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);
                    if( HIWORD( wParam ) == BN_CLICKED )
                    {
                      switch (LOWORD(wParam))
                      {
	                      case IDC_QUICK_COMPRESS_CHECK:
		                      CompConfig->QuickCompress = (CompConfig->QuickCompress) ? 0 : 1;
  		                      CheckDlgButton( hWndDlg, IDC_QUICK_COMPRESS_CHECK, (CompConfig->QuickCompress) ? 1 : 0 );
		                      break;
                      }
                    }
                    break;

                case IDC_AUTOKEYFRAME_CHECK:
                    CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);
                    if( HIWORD( wParam ) == BN_CLICKED )
                    {
                        switch (LOWORD(wParam))
                        {
                            case IDC_AUTOKEYFRAME_CHECK:
                                CompConfig->AutoKeyFrameEnabled = (CompConfig->AutoKeyFrameEnabled) ? 0 : 1;
                                CheckDlgButton( hWndDlg, IDC_AUTOKEYFRAME_CHECK, (CompConfig->AutoKeyFrameEnabled) ? 1 : 0 );

                                EnableWindow(GetDlgItem(hWndDlg, IDC_EDIT_DIFF), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_EDIT_MIN), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_EDIT_MAX), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_SLIDER_DIFF), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_SLIDER_MIN), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_SLIDER_MAX), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_STATIC_DIFF), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_STATIC_MIN), CompConfig->AutoKeyFrameEnabled);
                                EnableWindow(GetDlgItem(hWndDlg, IDC_STATIC_MAX), CompConfig->AutoKeyFrameEnabled);

		                      break;
                      }
                    }
                    break;
                
                case IDC_EDIT_DIFF:
                case IDC_EDIT_MIN:
                case IDC_EDIT_MAX:
                    CompConfig = (COMP_CONFIG *)GetWindowLong(hWndDlg,GWL_USERDATA);
                    switch(HIWORD(wParam))
                    {
                        int minValue;
                        int maxValue;
                        int *valueToChange;
                        int editValue;

                        case EN_KILLFOCUS:
                            editValue = GetDlgItemInt(hWndDlg, LOWORD(wParam),  NULL, FALSE );

                            if(LOWORD(wParam) == IDC_EDIT_DIFF)
                            {
                                minValue = MIN_SENSITIVITY;
                                maxValue = MAX_SENSITIVITY;
                                hwndTrack = GetDlgItem(hWndDlg, IDC_SLIDER_DIFF);
                                valueToChange = (int *)&CompConfig->AutoKeyFrameThreshold;
                            }
                            else if(LOWORD(wParam) == IDC_EDIT_MIN)
                            {
                                minValue = MIN_MINIMUM;
                                maxValue = MAX_MINIMUM;
                                hwndTrack = GetDlgItem(hWndDlg, IDC_SLIDER_MIN);
                                valueToChange = (int *)&CompConfig->MinimumDistanceToKeyFrame;
                            }
                            else if(LOWORD(wParam) == IDC_EDIT_MAX)
                            {
                                minValue = MIN_MAXIMUM;
                                maxValue = MAX_MAXIMUM;
                                hwndTrack = GetDlgItem(hWndDlg, IDC_SLIDER_MAX);
                                valueToChange = (int *)&CompConfig->ForceKeyFrameEvery;
                            }

                            if((editValue < minValue) || (editValue > maxValue))
                            {
                                char msg[256];
                                sprintf(msg,"Please enter a value between %d and %d.", 
                                                minValue, maxValue);
                                MessageBox(hWndDlg, msg, "Configuration Input Error",
                                             MB_ICONERROR);

                                SetFocus((HWND) lParam);

                                SendMessage(GetDlgItem(hWndDlg, LOWORD(wParam)), 
                                            EM_SETSEL, (WPARAM) 0, (LPARAM) -1);

                            }
                            else
                            {
                                //update the config struct 
                                *valueToChange = editValue;

                                SendMessage(hwndTrack, TBM_SETPOS, (WPARAM) TRUE, 
                                            (LPARAM) editValue);

                                //extra logic to make sure the min slider is never greater than the max slider
                                if(LOWORD(wParam) == IDC_EDIT_MIN)
                                {
                                   if(*valueToChange > CompConfig->ForceKeyFrameEvery)
                                    {
                                        CompConfig->ForceKeyFrameEvery = *valueToChange;
                                        SetDlgItemInt( hWndDlg, IDC_EDIT_MAX, CompConfig->ForceKeyFrameEvery, FALSE );

                                        SendMessage(GetDlgItem(hWndDlg, IDC_SLIDER_MAX), TBM_SETPOS, (WPARAM) TRUE, 
                                                        (LPARAM) *valueToChange);
                                    }
                                }
                                else if(LOWORD(wParam) == IDC_EDIT_MAX)
                                {
                                    if(*valueToChange < (int)CompConfig->MinimumDistanceToKeyFrame)
                                    {
                                        CompConfig->MinimumDistanceToKeyFrame = *valueToChange;
                                        SetDlgItemInt( hWndDlg, IDC_EDIT_MIN, CompConfig->MinimumDistanceToKeyFrame, FALSE );

                                        SendMessage(GetDlgItem(hWndDlg, IDC_SLIDER_MIN), TBM_SETPOS, (WPARAM) TRUE, 
                                                        (LPARAM) *valueToChange);
                                    }
                                }

                            }
                            break;
                    }
                    break;
            default:
                return (FALSE);

          }
          return (TRUE);
       
        default:
            return (FALSE);

     } /* End of Main Dialog case statement. */

}     /* End of WndProc                                         */
