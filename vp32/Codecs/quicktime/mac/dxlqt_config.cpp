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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <Dialogs.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>

#include "dxlqt_config.h"


namespace 
{
    const int MIN_SENSITIVITY = 50;
    const int MAX_SENSITIVITY = 100;

    const int MIN_MINIMUM = 0;
    const int MAX_MINIMUM = 30;

    const int MIN_MAXIMUM = 0;
    const int MAX_MAXIMUM = 600;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* This userItem dims the bottom edit line if the check box is not checked */
pascal	 void 
//dxlqtConfigDialog::
DimEditLine(WindowPtr dwind, short dinum)
{
#if 0
    ControlHandle tempCont;
    
	    GetDialogItem(dwind, CheckAutoKeyID, NULL, &(Handle)tempCont, NULL);
//    tempCont = SnatchHandle(dwind, CheckAutoKeyID);
    /* only do it if the checkbox is false */
    if (GetControlValue(tempCont) == false) 
    {
        PenState thePen;
        short itemType;
        Handle itemHandle;
        Rect dimRect;
        /* Save and restore the pen state so we don't mess things up for other */
        /* drawing routines */
        GetPenState(&thePen);
        GetDialogItem(dwind, dinum, &itemType, &itemHandle, &dimRect);
//        PenMode(notPatBic);
        PenMode(notPatBic);
        PenPat(&qd.gray);
        PaintRect(&dimRect);
        SetPenState(&thePen);
    }
#endif
} /* end DimEditLine */

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::InitEditControl(DialogPtr theDialog, short theItem, unsigned int theValue)
{
	Str255 	tempStr;

	short iType;
	Handle theControl;
	Rect iRect;
	GrafPtr savePort;
	Size mySize;
	
	GetPort(&savePort);
	SetPort(theDialog);
		
	GetDialogItem(theDialog, theItem, &iType, &theControl, &iRect);

    //lets setup the edit box to center and turn off scrolling
    (**(((DialogPeek)theDialog)->textH)).destRect = (**(((DialogPeek)theDialog)->textH)).viewRect;                  
    TESetAlignment(teCenter,(((DialogPeek)theDialog)->textH));
    TEAutoView(false,(((DialogPeek)theDialog)->textH));					

    //now put in the text
    NumToString(theValue, tempStr);
    
    SetDialogItemText(theControl, tempStr);

	SetPort(savePort); 
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dialog filter event handler for keypresses. 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Boolean 
dxlqtConfigDialog::HandleKeyPress(DialogPtr theDialog, char theChar)
{
    Boolean rv;
    TEHandle theTE = ((DialogPeek)theDialog)->textH;
    
    switch(theChar)
    {
        case '0': 
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            //make sure the user does not exceed the limit
            if(((**theTE).teLength - ((**theTE).selEnd - (**theTE).selStart) + 1 < kMaxEditLength) 
                && AutoKeyFrameEnabled)
            {
                rv = false;
            }
            else
            {
                SysBeep(1);
                rv = true;
            }
            break;

        case kBackspaceCharCode:
        case kLeftArrowCharCode:
        case kRightArrowCharCode:
            rv = false;
            break;
            
        default:
            SysBeep(1);
            rv = true;
            break;
    }

    return rv;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Boolean 
dxlqtConfigDialog::ValidateCurrentEntry(DialogPtr theDialog, unsigned int lastEditID)
{
    if(lastEditID != 0)
    {
        unsigned int * valueToUpdate;
        int 			minValue;
        int 			maxValue;
    
        if(lastEditID == AutoKeyTextBoxID)
        {
            minValue = MIN_SENSITIVITY;
            maxValue = MAX_SENSITIVITY;
            valueToUpdate = &AutoKeyFrameThreshold;
        }
        else if(lastEditID == MinDistTextBoxID)
        {
            minValue = MIN_MINIMUM;
            maxValue = MAX_MINIMUM;
            valueToUpdate = &MinimumDistanceToKeyFrame;
        }
        else if(lastEditID == ForceKeyTextBoxID)
        {
            minValue = MIN_MAXIMUM;
            maxValue = MAX_MAXIMUM;
            valueToUpdate = &ForceKeyFrameEvery;
        }
        
    	//ok now we have the user entered value, so lets check
    	//to see if its within the valid range
        long theValue;
        theValue = GetTextBoxValue(lastControlEditID);

        if((theValue >= minValue) && (theValue <= maxValue))
        {
            *valueToUpdate = theValue;
        }
        else
        { 
            short item;
            Str255 errorMsg;
            
            sprintf((char *)&errorMsg[1], 
                    "Please enter a value between %d and %d.",
                     minValue, maxValue);
            errorMsg[0] = (char)strlen((char *)&errorMsg[1]);                     
            
            SysBeep(0);
            StandardAlert (kAlertStopAlert, "\pConfiguration Input Error",
					errorMsg,
					NULL, &item);
        
            //lets hilite the entire entry
            SelectDialogItemText(theDialog, lastControlEditID, 0, 32767);
            return false;
        }
    }
    return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::UpdateSlider(DialogPtr theDialog, unsigned int lastEditID)
{
    Handle tempHand;
    int theValue;

    switch(lastEditID)
    {
        case AutoKeyTextBoxID:
            GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
            SetControlValue((ControlHandle)tempHand, AutoKeyFrameThreshold);
            break;

        case MinDistTextBoxID:
            GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
            SetControlValue((ControlHandle)tempHand, MinimumDistanceToKeyFrame);
            
            //now we have to make sure that the max kf value is not less than the min kf value

            //now update the max slider if necessary
		    GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
		    theValue = GetControlValue((ControlHandle)tempHand);

            //we cant have a max keyframe value that is less than the min keyframe value
            if(MinimumDistanceToKeyFrame > theValue)
            {
    		    ForceKeyFrameEvery = MinimumDistanceToKeyFrame;
                SetControlValue((ControlHandle)tempHand, ForceKeyFrameEvery);
                UpdateItemText(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);
                
                //set the focus back
                SelectDialogItemText(theDialog, MinDistTextBoxID, 0, 0);
                
                //refresh
                DrawDialog(theDialog);                
            }
            break;
            
        case ForceKeyTextBoxID:
            GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
            SetControlValue((ControlHandle)tempHand, ForceKeyFrameEvery);


            //now update the min slider if necessary
    	    GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
    	    theValue = GetControlValue((ControlHandle)tempHand);
            
            //we cant have a max keyframe value that is less than the min keyframe value
            if(ForceKeyFrameEvery < theValue)
            {
    		    MinimumDistanceToKeyFrame = ForceKeyFrameEvery;
                SetControlValue((ControlHandle)tempHand, MinimumDistanceToKeyFrame);
                UpdateItemText(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
                
                //set the focus back
                SelectDialogItemText(theDialog, ForceKeyTextBoxID, 0, 0);
    
                //refresh
                DrawDialog(theDialog);                
            }

            break;
    }
}   

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Boolean 
dxlqtConfigDialog::HandleMouse(DialogPtr theDialog, Point pt, short modifiers)
{
	short iType;
	Handle iHndl;
	Rect diffRect;
	Rect minRect;
	Rect maxRect; 
	Rect cancelRect;
	Rect defaultRect;
	
	GrafPtr savePort;
	Boolean shiftDown;
	Boolean isThisMouseDownForTE = false;
	
	GetPort(&savePort);
	SetPort(theDialog);
	
	shiftDown = modifiers & shiftKey;
	GlobalToLocal(&pt);
	
	GetDialogItem(theDialog, ButtonCancelID, &iType, &iHndl, &cancelRect);
	GetDialogItem(theDialog, ButtonDefaultID, &iType, &iHndl, &defaultRect);
	
	GetDialogItem(theDialog, AutoKeyTextBoxID, &iType, &iHndl, &diffRect);
	GetDialogItem(theDialog, MinDistTextBoxID, &iType, &iHndl, &minRect);
	GetDialogItem(theDialog, ForceKeyTextBoxID, &iType, &iHndl, &maxRect);


//DialogPeek aJunk = ((DialogPeek)theDialog);

    //if the user hits cancel or default settings then abort mission
   	if (PtInRect(pt,&cancelRect) || PtInRect(pt,&defaultRect)) 
	{
    	lastControlEditID = 0;	
        return false;
	}

    if(AutoKeyFrameEnabled)
    {
    	if (PtInRect(pt,&diffRect)) 
    	{
            if(lastControlEditID != AutoKeyTextBoxID)
            {
                if(ValidateCurrentEntry(theDialog, lastControlEditID))
                {
                    UpdateSlider(theDialog, lastControlEditID);
                	lastControlEditID = AutoKeyTextBoxID;
                	kMaxEditLength = 4;
                }
                else
                {
        		    //this will force the user to enter a valid value
                    isThisMouseDownForTE = true;
                }
            }
    	}
    	else if (PtInRect(pt,&minRect)) 
    	{
            if(lastControlEditID != MinDistTextBoxID)
            {
                if(ValidateCurrentEntry(theDialog, lastControlEditID))
                {
                    UpdateSlider(theDialog, lastControlEditID);
                	lastControlEditID = MinDistTextBoxID;
                	kMaxEditLength = 3;
                }
                else
                {
        		    //this will force the user to enter a valid value
                    isThisMouseDownForTE = true;
                }
            }
    	}
    	else if (PtInRect(pt,&maxRect)) 
    	{
            if(lastControlEditID != ForceKeyTextBoxID)
            {
                if(ValidateCurrentEntry(theDialog, lastControlEditID))
                {
                    UpdateSlider(theDialog, lastControlEditID);
                	lastControlEditID = ForceKeyTextBoxID;
                	kMaxEditLength = 4;
                }
                else
                {
        		    //this will force the user to enter a valid value
                    isThisMouseDownForTE = true;
                }
            }
    	}
    	else
    	{
            if(ValidateCurrentEntry(theDialog, lastControlEditID))
            {
                UpdateSlider(theDialog, lastControlEditID);
//            	lastControlEditID = 0;	
        		isThisMouseDownForTE = false;
            }   
            else
            { 
    		    //this will force the user to enter a valid value
    		    isThisMouseDownForTE = true;
    		}
    	}
    }
    else
    {
    
    	if (PtInRect(pt,&diffRect) || PtInRect(pt,&minRect) || PtInRect(pt,&maxRect))
    	    //do not allow mouse clicks on disabled items 
    		isThisMouseDownForTE = true;
        else
    		isThisMouseDownForTE = false;
    }
    		
	SetPort(savePort);
	
	return isThisMouseDownForTE;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// this is the main dispatcher for events to be passed off to 
// the textedit box. looks sort of like a WaitNextEvent event handler.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
pascal Boolean 
dxlqtConfigDialog::MyDialogFilter(DialogPtr theDialog,EventRecord *ev,short *itemHit)
{
	#pragma unused(itemHit)
	char theChar;
    long thisAsLong = GetWRefCon(theDialog); 	
	dxlqtConfigDialog* const configDialogPtr = reinterpret_cast<dxlqtConfigDialog*>(thisAsLong);
	 
	switch (ev->what) 
	{
		case keyDown:
		case autoKey:
			theChar = (ev->message & charCodeMask);
			return configDialogPtr->HandleKeyPress(theDialog,theChar);
		    
		case activateEvt:
			return false;

		case mouseDown:
			return configDialogPtr->HandleMouse(theDialog,ev->where,ev->modifiers);

		case nullEvent:
		case updateEvt:
		default:
			return false;
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
dxlqtConfigDialog::dxlqtConfigDialog(dxlqt_Globals glob)	
{
    HLock((Handle)glob);
    HLock((Handle)internalGlob);

    internalGlob = glob;
    Settings = &(**internalGlob).CompConfig;
	InitSettings();
	// call dialog/do all our work
    DisplayDialog();
    
    HUnlock((Handle)glob);
    HUnlock((Handle)internalGlob);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::InitSettings()
{
//    lastControlEditID = 0;

    AutoKeyFrameThreshold = Settings->AutoKeyFrameThreshold;
    MinimumDistanceToKeyFrame = Settings->MinimumDistanceToKeyFrame;
    ForceKeyFrameEvery = Settings->ForceKeyFrameEvery;
    
    AutoKeyFrameEnabled = Settings->AutoKeyFrameEnabled;
    AllowDF = Settings->AllowDF;
    QuickCompress = Settings->QuickCompress;
}
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::SetDefaults()
{
    COMP_CONFIG     default_settings;
    
    getCompConfigDefaultSettings(&default_settings);
    
    // Slider/text vals
    AutoKeyFrameThreshold = default_settings.AutoKeyFrameThreshold;
    MinimumDistanceToKeyFrame = default_settings.MinimumDistanceToKeyFrame;
    ForceKeyFrameEvery = default_settings.ForceKeyFrameEvery;
    
    // check boxes
    AutoKeyFrameEnabled = default_settings.AutoKeyFrameEnabled;
    AllowDF = default_settings.AllowDF;
    QuickCompress = default_settings.QuickCompress;
    
    
    InitDialogControlSettings();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::InitDialogControlSettings()
{
	Handle 	tempHand;
	Str255 	tempStr;
	short   tempItem;
	Rect    tempRect;

	if(theDialog != nil)
	{ 

        InitEditControl(theDialog, AutoKeyTextBoxID, AutoKeyFrameThreshold);
        InitEditControl(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
        InitEditControl(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);


//need to fix this to point to end of edit box
//have to do this for some reason
SelectDialogItemText(theDialog, ForceKeyTextBoxID, 0, 0);
SelectDialogItemText(theDialog, MinDistTextBoxID, 0, 0);
SelectDialogItemText(theDialog, AutoKeyTextBoxID, 0, 0);
lastControlEditID = AutoKeyTextBoxID;
kMaxEditLength = 4;

	    // Set default buttons
	    // SetDialogDefaultItem(theDialog, ButtonOKID);
	    SetDialogCancelItem(theDialog,ButtonCancelID);

	    // Set AutoKeyThreshold Slider/Text Box (%Difference.. on screen)
	    GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
	    SetControlValue((ControlHandle)tempHand, AutoKeyFrameThreshold);

	    // Set Minimum Dist to Key Slider/Text Box (Min # Frames.. on screen)
	    GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
	    SetControlValue((ControlHandle)tempHand, MinimumDistanceToKeyFrame);
	
	    // Set Force Key Frame Slider/Text Box (Max # Frames.. on screen)
	    GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
	    SetControlValue((ControlHandle)tempHand, ForceKeyFrameEvery);

        // Set AutoKey Check Box
        GetDialogItem(theDialog, CheckAutoKeyID, NULL, &tempHand, NULL);
        SetControlValue((ControlHandle)tempHand, AutoKeyFrameEnabled); 



        /* set up the user item that will dim the edit line when it is disabled */
        GetDialogItem(theDialog, kDimmingBox, &tempItem, &tempHand, &tempRect);
        SetDialogItem(theDialog, kDimmingBox, tempItem, (Handle)(NewUserItemProc(DimEditLine)), &tempRect);

        if(AutoKeyFrameEnabled)
        {
            GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
            ActivateControl((ControlHandle)tempHand);

            GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
            ActivateControl((ControlHandle)tempHand);

            GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
            ActivateControl((ControlHandle)tempHand);
        }
        else
        {
            GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
            DeactivateControl((ControlHandle)tempHand);

            GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
            DeactivateControl((ControlHandle)tempHand);

            GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
            DeactivateControl((ControlHandle)tempHand);

            GetDialogItem(theDialog, kDimmingBox, &tempItem, &tempHand, &tempRect);
            InvalRect(&tempRect);
        }    
        
        DrawDialog(theDialog);  
                 
        // Set Allow Dropped Frames Check Box
        GetDialogItem(theDialog, CheckAllowDFID, NULL, &tempHand, NULL);
        SetControlValue((ControlHandle)tempHand, AllowDF);
    
        // Set Quick Compress Check Box
        GetDialogItem(theDialog, CheckQuickCompressID, NULL, &tempHand, NULL);
        SetControlValue((ControlHandle)tempHand, QuickCompress);   
    }
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::updateSettings()
{
    // hardcoded as requested - tjf
    Settings->NoiseSensitivity = 1;
    Settings->Quality = 56;
    Settings->AutoKeyFrameThreshold = AutoKeyFrameThreshold;
    Settings->MinimumDistanceToKeyFrame = MinimumDistanceToKeyFrame;
    Settings->ForceKeyFrameEvery = ForceKeyFrameEvery;
	
    Settings->AutoKeyFrameEnabled = AutoKeyFrameEnabled;    
    Settings->AllowDF = AllowDF;
    Settings->QuickCompress = QuickCompress;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int 
dxlqtConfigDialog::GetTextBoxValue(int textBoxID)
{
    Str255 	tempStr;
    Handle 	tempHand;
    long	tempVal;
	
    GetDialogItem(theDialog, textBoxID, NULL, &tempHand, NULL);
    GetDialogItemText(tempHand, tempStr);
    StringToNum(tempStr, &tempVal);
	
    return tempVal;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::UpdateItemText(DialogPtr theDialog, short theItem, int theValue)
{
    Str255 	tempStr;
    Handle 	tempHand;
    Rect    tempRect;
    short   iType;

    NumToString(theValue, tempStr);

    GetDialogItem(theDialog, theItem, &iType, &tempHand, &tempRect);
    SetDialogItemText(tempHand, tempStr);

	TEHandle theTE = ((DialogPeek)theDialog)->textH;
	TEUpdate(&tempRect, theTE);

    SelectDialogItemText(theDialog, theItem, 0, 0);
//DrawDialog(theDialog);
}
	
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::DisplayDialog()
{
    ControlHandle       slideHand;
    Handle              textHand, tempHand, tempCntlHand;
    int                 theValue;
    int                 minValue;
    int 			    maxValue;
    Str255			    tempStr;
	Rect                tempRect;
	short               tempItem;
	
    DialogItemIndex     itemHit;
	
    GrafPtr 		    savePort;
    short 			    saveResFile;
    OSStatus  		    errAppear;
    OSErr               myErr;
	
    saveResFile = CurResFile();	
    myResFile = OpenComponentResFile((Component)(**internalGlob).regAccess->theCodec);
//myResFile = saveResFile;

//    myErr = OpenAComponentResFile((Component)internalGlob->regAccess->theCodec, (short*)myResFile);
	
    // Open the dialog
    theDialog = GetNewDialog(dxlqtConfigDialogID, NULL, (WindowPtr)-1L);
	
	//save (this) so we can access it during the MyDialogFilter callback
	SetWRefCon(theDialog, reinterpret_cast<long>(this));
	
    // Prepare the dialog items
    InitDialogControlSettings();
	
    // Show the window and process its events
    ShowWindow(theDialog);
    GetPort(&savePort);
    SetPort(theDialog);

    //show it once
    DrawDialog(theDialog);
	
    Finished = false;
    itemHit = 0;
	
    while(Finished != true)
    {
        ModalDialog(NewModalFilterProc(MyDialogFilter), &itemHit);

	    // all other events fall to here
	    switch(itemHit) 
	    {
    	    case ButtonCancelID:
    		    Finished = true;
    		    break;
    	
    	    case ButtonOKID:
    		    Finished = true;
    		    updateSettings();
    		    break;
    		
    	    case ButtonDefaultID:
    	        SetDefaults();
    	        break;
    	
    	    case AutoKeyThresholdSliderID:
    		    GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
    		    AutoKeyFrameThreshold = GetControlValue((ControlHandle)tempHand);
                UpdateItemText(theDialog, AutoKeyTextBoxID, AutoKeyFrameThreshold);
    		    break;
    
    	    case MinDistToKeySliderID:
    		    GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
    		    MinimumDistanceToKeyFrame = GetControlValue((ControlHandle)tempHand);
                UpdateItemText(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
                
                //now update the max slider if necessary
    		    GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
    		    theValue = GetControlValue((ControlHandle)tempHand);

                //we cant have a max keyframe value that is less than the min keyframe value
                if(MinimumDistanceToKeyFrame > theValue)
                {
        		    ForceKeyFrameEvery = MinimumDistanceToKeyFrame;
                    SetControlValue((ControlHandle)tempHand, ForceKeyFrameEvery);
                    UpdateItemText(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);
                    
                }
                //set the focus back
                SelectDialogItemText(theDialog, MinDistTextBoxID, 0, 0);
                
                //refresh
                DrawDialog(theDialog);                
    		    break;
    		
    	    case ForceKeyFrameEverySliderID:
    		    GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
    		    ForceKeyFrameEvery = GetControlValue((ControlHandle)tempHand);
                UpdateItemText(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);
                
                //now update the min slider if necessary
    		    GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
    		    theValue = GetControlValue((ControlHandle)tempHand);
                
                //we cant have a max keyframe value that is less than the min keyframe value
                if(ForceKeyFrameEvery < theValue)
                {
        		    MinimumDistanceToKeyFrame = ForceKeyFrameEvery;
                    SetControlValue((ControlHandle)tempHand, MinimumDistanceToKeyFrame);
                    UpdateItemText(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
                }
                    
                //set the focus back
                SelectDialogItemText(theDialog, ForceKeyTextBoxID, 0, 0);

                //refresh
                DrawDialog(theDialog);                
    		    break;
    
    	    case CheckAutoKeyID:
    		    GetDialogItem(theDialog, itemHit, NULL, &tempHand, NULL);
    		    theValue = GetControlValue((ControlHandle)tempHand);
    		    theValue = !theValue;
    		    AutoKeyFrameEnabled = (Boolean)theValue;
    		    SetControlValue((ControlHandle)tempHand, theValue);
    
    
                if(theValue)
                {
                    GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
                    ActivateControl((ControlHandle)tempHand);
    
                    GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
                    ActivateControl((ControlHandle)tempHand);
    
                    GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
                    ActivateControl((ControlHandle)tempHand);
                }
                else
                {
                    GetDialogItem(theDialog, AutoKeyThresholdSliderID, NULL, &tempHand, NULL);
                    DeactivateControl((ControlHandle)tempHand);
    
                    GetDialogItem(theDialog, MinDistToKeySliderID, NULL, &tempHand, NULL);
                    DeactivateControl((ControlHandle)tempHand);
    
                    GetDialogItem(theDialog, ForceKeyFrameEverySliderID, NULL, &tempHand, NULL);
                    DeactivateControl((ControlHandle)tempHand);
    
                    GetDialogItem(theDialog, kDimmingBox, &tempItem, &tempHand, &tempRect);
                    InvalRect(&tempRect);
                }    
                
                DrawDialog(theDialog);    
    		    break;
    
    	
    	    case CheckAllowDFID:
    		    GetDialogItem(theDialog, itemHit, NULL, &tempHand, NULL);
    		    theValue = GetControlValue((ControlHandle)tempHand); 
    		    theValue = !theValue;
    	        AllowDF = (Boolean)theValue;			
    		    SetControlValue((ControlHandle)tempHand, theValue);
    		    break;
    	
    	    case CheckQuickCompressID:
    		    GetDialogItem(theDialog, itemHit, NULL, &tempHand, &tempRect);
    		    theValue = GetControlValue((ControlHandle)tempHand); 
    		    theValue = !theValue;
    		    QuickCompress = (Boolean)theValue;
    		    SetControlValue((ControlHandle)tempHand, (unsigned char)theValue);
    		    
    		    break;

	    } // switch(itemHit)


	} // while(Finished != true)
	
	// Close the dialog
	DisposeDialog(theDialog);
	
	//DisposeRoutineDescriptor((UniversalProcPtr)filterUPP);
	SetPort(savePort);
	CloseComponentResFile(myResFile);
	UseResFile(saveResFile);
	
}
