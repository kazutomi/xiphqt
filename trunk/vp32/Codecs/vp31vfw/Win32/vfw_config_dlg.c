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
*
*****************************************************************************
*/						

/****************************************************************************
*  Header Frames
*****************************************************************************
*/

#define STRICT              /* Strict type checking. */
#include <windows.h>
#include <stdio.h>  

#define INC_WIN_HEADER

#include "type_aliases.h"
#include "vfw_comp_interface.h"
#include "resource.h"		// Must be the version resident in the pre-processor dll directory!!!



/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

#define QUAL_ENTRIES    64
#define MIN_Q_THRESH    0
#define MAX_Q_THRESH    (QUAL_ENTRIES - 1) 

#define MIN_DRATE       0
#define MAX_DRATE       2048 

#define MIN_KF_DRATE    0
#define MAX_KF_DRATE    400

#define MIN_SENSITIVITY 50
#define MAX_SENSITIVITY 100

#define MIN_MINIMUM		0
#define MAX_MINIMUM     30

#define MIN_MAXIMUM		0
#define MAX_MAXIMUM     600

#define MIN_NOISE		0
#define MAX_NOISE       6

/****************************************************************************
*  Imported Global Variables
*****************************************************************************
*/

extern char  WindowsSystemPath[_MAX_PATH];
extern char  ParamsFilePath[_MAX_PATH + 12];

extern COMP_CONFIG CompConfig;


/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

/****************************************************************************
*  Foreward References
*****************************************************************************
*/              


/****************************************************************************
*  Module Statics 
*****************************************************************************
*/              

/**/
/************************************************************************/
/*                                                                      */
/* PP Setup Dialog Call Back                                          */
/*                                                                      */
/*                                                                      */
/************************************************************************/

BOOL FAR PASCAL Config_ParamsDlgProc(   HWND   hWndDlg,
                                        UINT   Message,
                                        WPARAM wParam,
                                        LPARAM lParam )
{
     BOOL ret_val = TRUE;             /* Returned status. */
     int                              ScrollPosition;
     int                              MaxSBPos;
     int                              MinSBPos;
     SCROLLINFO ScrollBarInfo;
     int  PageStep = 2;

     /* Variables to note values on input in case the user 
     *  wants to cancel. 
     */
     static int		TempDRate, TempQuality, TempKFDRate;
	 static BOOL	TempAllowDroppedFrames = FALSE;	
	 static BOOL	TempQuickCompress = TRUE;	
	 static BOOL    TempAutoKeyFrameEnabled;
	 static INT32   TempAutoKeyFrameThreshold;
	 static UINT32  TempMinimumDistanceToKeyFrame;
	 static INT32   TempForceKeyFrameEvery;

	 static INT32   TempNoiseSensitivity;

     switch(Message)
     {        
     case WM_INITDIALOG:

          /* Note values on entry. */
          TempDRate = CompConfig.TargetBitRate;
          TempKFDRate = CompConfig.KeyFrameDataTarget;
          TempQuality = (MAX_Q_THRESH - CompConfig.Quality);
		  TempAllowDroppedFrames = CompConfig.AllowDF;	
          TempQuickCompress = CompConfig.QuickCompress;
		  TempAutoKeyFrameEnabled		= CompConfig.AutoKeyFrameEnabled;
		  TempAutoKeyFrameThreshold		= CompConfig.AutoKeyFrameThreshold;
		  TempMinimumDistanceToKeyFrame = CompConfig.MinimumDistanceToKeyFrame;
		  TempForceKeyFrameEvery		= CompConfig.ForceKeyFrameEvery;
		  TempNoiseSensitivity			= CompConfig.NoiseSensitivity;

		  //  Set check boxes.
  		  CheckDlgButton( hWndDlg, IDC_ALLOW_DROPPED_FRAMES_CHECK, (CompConfig.AllowDF) ? 1 : 0 );
  		  CheckDlgButton( hWndDlg, IDC_QUICK_COMPRESS_CHECK, (CompConfig.QuickCompress) ? 1 : 0 );
  		  CheckDlgButton( hWndDlg, IDC_AUTOKEYFRAME_CHECK, (CompConfig.AutoKeyFrameEnabled) ? 1 : 0 );


          /* Fixed elements of ScrollBarInfo */
          ScrollBarInfo.cbSize = 28;
          ScrollBarInfo.fMask = SIF_POS | SIF_RANGE;

          /* Quality Scrollbar. */          
          ScrollBarInfo.nMin = MIN_Q_THRESH;
          ScrollBarInfo.nMax = MAX_Q_THRESH;
          ScrollBarInfo.nPos = (MAX_Q_THRESH - CompConfig.Quality);
          SetScrollInfo( GetDlgItem(hWndDlg, ID_QUALITY_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_ID_QUALITY_EDIT, CompConfig.Quality + 1, FALSE );          // 1 based not 0 based

          /* Data Rate Scroll Bar */          
          ScrollBarInfo.nMin = MIN_DRATE;
          ScrollBarInfo.nMax = MAX_DRATE;
          ScrollBarInfo.nPos = CompConfig.TargetBitRate;
          SetScrollInfo( GetDlgItem(hWndDlg, ID_DATA_RATE_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_DATA_RATE_EDIT, ScrollBarInfo.nPos, FALSE );

          /* Key Frame Data Rate Scroll Bar */          
          ScrollBarInfo.nMin = MIN_KF_DRATE;
          ScrollBarInfo.nMax = MAX_KF_DRATE;
          ScrollBarInfo.nPos = CompConfig.KeyFrameDataTarget;
          SetScrollInfo( GetDlgItem(hWndDlg, ID_KF_DATA_RATE_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_KF_DATA_RATE_EDIT, ScrollBarInfo.nPos, FALSE );
          
          /* Key Frame Sensitivity */          
          ScrollBarInfo.nMin = MIN_SENSITIVITY;
          ScrollBarInfo.nMax = MAX_SENSITIVITY;
          ScrollBarInfo.nPos = CompConfig.AutoKeyFrameThreshold;
          SetScrollInfo( GetDlgItem(hWndDlg, ID_SENSITIVITY_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_SENSITIVITY_EDIT, ScrollBarInfo.nPos, FALSE );

          /* Minimum # of frames in between keyframes */          
          ScrollBarInfo.nMin = MIN_MINIMUM;
          ScrollBarInfo.nMax = MAX_MINIMUM;
          ScrollBarInfo.nPos = CompConfig.MinimumDistanceToKeyFrame;
          SetScrollInfo( GetDlgItem(hWndDlg, ID_MINIMUM_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_MINIMUM_EDIT, ScrollBarInfo.nPos, FALSE );


          /* MAXIMUM # of frames in between keyframes */          
          ScrollBarInfo.nMin = MIN_MAXIMUM;
          ScrollBarInfo.nMax = MAX_MAXIMUM;
          ScrollBarInfo.nPos = CompConfig.ForceKeyFrameEvery;
          SetScrollInfo( GetDlgItem(hWndDlg, ID_MAXIMUM_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_MAXIMUM_EDIT, ScrollBarInfo.nPos, FALSE );

          /* Noise Filter Level */          
          ScrollBarInfo.nMin = MIN_NOISE;
          ScrollBarInfo.nMax = MAX_NOISE;
          ScrollBarInfo.nPos = CompConfig.NoiseSensitivity;
          SetScrollInfo( GetDlgItem(hWndDlg, ID_NOISE_SCROLL_BAR), SB_CTL, &ScrollBarInfo, TRUE );
          SetDlgItemInt( hWndDlg, IDC_NOISE_EDIT, ScrollBarInfo.nPos, FALSE );

     case WM_HSCROLL:


          /* Set the page up and down increment. */
		  if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_QUALITY_SCROLL_BAR) )
          {
              PageStep = 1;
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_DATA_RATE_SCROLL_BAR) )
          {
              PageStep = 16;
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_KF_DATA_RATE_SCROLL_BAR) )
          {
              PageStep = 10;
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_SENSITIVITY_SCROLL_BAR) )
          {
              PageStep = 1;
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_MINIMUM_SCROLL_BAR) )
          {
              PageStep = 3;
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_MAXIMUM_SCROLL_BAR) )
          {
              PageStep = 60;
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_NOISE_SCROLL_BAR) )
          {
              PageStep = 1;
          }

          ScrollPosition = GetScrollPos( (HWND)lParam, SB_CTL );
          switch(LOWORD(wParam))
          {
          case SB_LINEUP:
                ScrollPosition--;
                break;
          case SB_LINEDOWN:
                ScrollPosition++;
                break;
          case SB_PAGEUP:
                ScrollPosition -= PageStep;
                break;
          case SB_PAGEDOWN:
                ScrollPosition += PageStep;
                break;
          case SB_THUMBPOSITION:
                ScrollPosition = HIWORD(wParam);
                break;
          }        
          
          /* Range check. */
          GetScrollRange( (HWND)lParam, SB_CTL, &MinSBPos, &MaxSBPos );
      
          if ( ScrollPosition > MaxSBPos )
                ScrollPosition = MaxSBPos;
          else if ( ScrollPosition <= MinSBPos )
                ScrollPosition = MinSBPos;

          /* Update the control and re-display. */
          SetScrollPos( (HWND)lParam, SB_CTL, ScrollPosition, TRUE );
          UpdateWindow( (HWND)lParam );
		  
		  if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_QUALITY_SCROLL_BAR) )
          {
              /* Set the quality variuable. */
              CompConfig.Quality = (MAX_Q_THRESH - ScrollPosition);
              SetDlgItemInt( hWndDlg, IDC_ID_QUALITY_EDIT, CompConfig.Quality + 1, FALSE );  // 1 based not 0 based
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_DATA_RATE_SCROLL_BAR) )
          {
              /* Set the data rate variuable (and edit box) */
              CompConfig.TargetBitRate = ScrollPosition;
              SetDlgItemInt( hWndDlg, IDC_DATA_RATE_EDIT, ScrollPosition, FALSE );
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_KF_DATA_RATE_SCROLL_BAR) )
          {
              /* Set the key frame data rate variuable (and edit box) */
              CompConfig.KeyFrameDataTarget = ScrollPosition;
              SetDlgItemInt( hWndDlg, IDC_KF_DATA_RATE_EDIT, ScrollPosition, FALSE );
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_SENSITIVITY_SCROLL_BAR) )
          {
              /* Set the sensitivity variuable (and edit box) */
              CompConfig.AutoKeyFrameThreshold = ScrollPosition;
              SetDlgItemInt( hWndDlg, IDC_SENSITIVITY_EDIT, ScrollPosition, FALSE );
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_MINIMUM_SCROLL_BAR) )
          {
              /* Set the mimimum distance to keyframe variuable (and edit box) */
              CompConfig.MinimumDistanceToKeyFrame = ScrollPosition;
              SetDlgItemInt( hWndDlg, IDC_MINIMUM_EDIT, ScrollPosition, FALSE );
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_MAXIMUM_SCROLL_BAR) )
          {
              /* Set the key frame every variable (and edit box) */
              CompConfig.ForceKeyFrameEvery = ScrollPosition;
              SetDlgItemInt( hWndDlg, IDC_MAXIMUM_EDIT, ScrollPosition, FALSE );
          }
		  else if ( ( HWND ) lParam == GetDlgItem(hWndDlg, ID_NOISE_SCROLL_BAR) )
          {
              /* Set the key frame every variable (and edit box) */
              CompConfig.NoiseSensitivity = ScrollPosition;
              SetDlgItemInt( hWndDlg, IDC_NOISE_EDIT, ScrollPosition, FALSE );
          }
		  break;
          
     case WM_CLOSE:          /* Close the dialog. */

          /* Closing the Dialog behaves the same as Cancel    */
          PostMessage(hWndDlg, WM_COMMAND, IDCANCEL, 0L);
          break;

      case WM_COMMAND:       /* A control has been activated. */
          switch(LOWORD(wParam))
          {

		  /* OK leaves the current settings in force */

          case IDOK:
               EndDialog(hWndDlg, IDOK);
               break; 

		  case IDCANCEL:

                /* Restore values on entry. */
                CompConfig.TargetBitRate = TempDRate;
                CompConfig.Quality = (MAX_Q_THRESH - TempQuality);
                CompConfig.KeyFrameDataTarget = TempKFDRate;
				CompConfig.AllowDF = TempAllowDroppedFrames;	
                CompConfig.QuickCompress = TempQuickCompress;
				CompConfig.AutoKeyFrameEnabled			= TempAutoKeyFrameEnabled		 ;
				CompConfig.AutoKeyFrameThreshold		= TempAutoKeyFrameThreshold		 ;
				CompConfig.MinimumDistanceToKeyFrame	= TempMinimumDistanceToKeyFrame  ;
				CompConfig.ForceKeyFrameEvery			= TempForceKeyFrameEvery		 ;
				CompConfig.NoiseSensitivity				= TempNoiseSensitivity			 ; 

				EndDialog(hWndDlg, IDCANCEL);
				break;

				
		  case IDC_ALLOW_DROPPED_FRAMES_CHECK:
		 	  if( HIWORD( wParam ) == BN_CLICKED )
			  {
				  switch (LOWORD(wParam))
				  {
					  case IDC_ALLOW_DROPPED_FRAMES_CHECK:
						  CompConfig.AllowDF = (CompConfig.AllowDF) ? 0 : 1;
  						  CheckDlgButton( hWndDlg, IDC_ALLOW_DROPPED_FRAMES_CHECK, (CompConfig.AllowDF) ? 1 : 0 );
						  break;
				  }
			  }
              break;

          case IDC_QUICK_COMPRESS_CHECK:
              if( HIWORD( wParam ) == BN_CLICKED )
              {
	              switch (LOWORD(wParam))
	              {
		              case IDC_QUICK_COMPRESS_CHECK:
			              CompConfig.QuickCompress = (CompConfig.QuickCompress) ? 0 : 1;
  			              CheckDlgButton( hWndDlg, IDC_QUICK_COMPRESS_CHECK, (CompConfig.QuickCompress) ? 1 : 0 );
			              break;
	              }
              }
              break;
		  case IDC_AUTOKEYFRAME_CHECK:
		 	  if( HIWORD( wParam ) == BN_CLICKED )
			  {
				  switch (LOWORD(wParam))
				  {
					  case IDC_AUTOKEYFRAME_CHECK:
						  CompConfig.AutoKeyFrameEnabled = (CompConfig.AutoKeyFrameEnabled) ? 0 : 1;
  						  CheckDlgButton( hWndDlg, IDC_AUTOKEYFRAME_CHECK, (CompConfig.AutoKeyFrameEnabled) ? 1 : 0 );
						  break;
				  }
			  }
              break;

          }
          break;

     default:
          ret_val = FALSE;

     }    /* End of Main Dialog case statement. */

     return ret_val;
     
}     /* End of WndProc                                         */
