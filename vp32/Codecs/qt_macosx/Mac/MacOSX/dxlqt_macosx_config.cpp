//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dxlqt_macosx_config.cpp
// 
// Purpose:  implementation of the class that runs the QT settings
//      dialog which opens off of the options button in QT's video compressor 
//      settings dialog.
// 
// Note:  This source file uses the OS X/carbon platform, it will not link
//      against InterfaceLib.  It probably won't even compile in CW 5.x.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <Dialogs.h>
#include <Fonts.h>
#include <Windows.h>
#include <TextEdit.h>

#include <string>

#include "dxlqt_config.h"
#include "duk_rsrc.h"

namespace 
{
    const int MIN_SENSITIVITY = 50;
    const int MAX_SENSITIVITY = 100;

    const int MIN_MINIMUM = 0;
    const int MAX_MINIMUM = 30;

    const int MIN_MAXIMUM = 0;
    const int MAX_MAXIMUM = 600;
}
using namespace std;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// dxlqtConfigDialog::dxlqtConfigDialog(dxlqt_Globals glob, ModalFilterUPP filterProc)
// Description: Constructor for Compressor settings dialog class
// Args:
// glob: pointer to global compressor settings
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
dxlqtConfigDialog::dxlqtConfigDialog(dxlqt_Globals glob)	
: m_bAutoTextFirstHit(false)
, m_bMinTextFirstHit(false)
, m_bForceTextFirstHit(false)
{
    HLock((Handle)glob);
    internalGlob = glob;
    HLock((Handle)internalGlob);
     
    Settings = &(**internalGlob).CompConfig;
	InitSettings();
	DisplayDialog();
    
    HUnlock((Handle)glob);
    HUnlock((Handle)internalGlob);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::osxErrorMsg(string& title, string& msg)
// Description: Takes strings, converts to pascal str format and displays
//      kAlertStopAlert alert box with title and msg as contents
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::osxErrorMsg(string& title, string& msg)
{
    char pTitleCStr[256];
    char pMsgCStr[256];
    short junkShort;
    
    if(title.length() < 256 && msg.length() < 256)
    {
        strcpy(pTitleCStr, title.c_str());
        strcpy(pMsgCStr, msg.c_str());
        CtoPStr(pTitleCStr);
        CtoPStr(pMsgCStr);
        StandardAlert(kAlertStopAlert, (unsigned char*)pTitleCStr, (unsigned char*)pMsgCStr, nil, &junkShort);
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::InitSettings()
// Description: Initializes the class members necessary for drawing the dialog 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::InitSettings()
{
    AutoKeyFrameThreshold = Settings->AutoKeyFrameThreshold;
    MinimumDistanceToKeyFrame = Settings->MinimumDistanceToKeyFrame;
    ForceKeyFrameEvery = Settings->ForceKeyFrameEvery;
    
    AutoKeyFrameEnabled = Settings->AutoKeyFrameEnabled;
    AllowDF = Settings->AllowDF;
    QuickCompress = Settings->QuickCompress;   
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::InitDialogControlSettings()
// Description: Preps the dialog for user interaction
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::InitDialogControlSettings()
{
	ControlHandle cH;
	DialogPtr theDialog = m_theDialog;
		
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
	    SetDialogDefaultItem(theDialog, ButtonOKID);
	    SetDialogCancelItem(theDialog,ButtonCancelID);
       
	    // Set AutoKeyThreshold Slider/Text Box (%Difference.. on screen)
        GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
        SetControlValue(cH, AutoKeyFrameThreshold);
        	    
	    // Set Minimum Dist to Key Slider/Text Box (Min # Frames.. on screen)
	    GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
	    SetControlValue(cH, MinimumDistanceToKeyFrame);
	    
	    // Set Force Key Frame Slider/Text Box (Max # Frames.. on screen)
	    GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
	    SetControlValue(cH, ForceKeyFrameEvery);

        // Set AutoKey Check Box
        GetDialogItemAsControl(theDialog, CheckAutoKeyID, &cH);
        SetControlValue(cH, AutoKeyFrameEnabled); 

        if(AutoKeyFrameEnabled)
        {
              // auto key slider
              GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
              ActivateControl(cH);

              // min dist slider
              GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
              ActivateControl(cH);

              // force key slider
              GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
              ActivateControl(cH);
              
              // AutoKeyTextBox
              GetDialogItemAsControl(theDialog, AutoKeyTextBoxID, &cH);
              ActivateControl(cH);
                                                    
              // MinDistTextBox
              GetDialogItemAsControl(theDialog, MinDistTextBoxID, &cH);
              ActivateControl(cH);
              
              // ForceKeyEveryTextBox
              GetDialogItemAsControl(theDialog, ForceKeyTextBoxID, &cH);
              ActivateControl(cH);
        }
        else
        {
               // auto key slider
               GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
               DeactivateControl(cH);

               // min dist slider
               GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
               DeactivateControl(cH);

               // force key slider
               GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
               DeactivateControl(cH);
                              
               // AutoKeyTextBox
               GetDialogItemAsControl(theDialog, AutoKeyTextBoxID, &cH);
               DeactivateControl(cH);
                                                     
               // MinDistTextBox
               GetDialogItemAsControl(theDialog, MinDistTextBoxID, &cH);
               DeactivateControl(cH);
               
               // ForceKeyEveryTextBox
               GetDialogItemAsControl(theDialog, ForceKeyTextBoxID, &cH);
               DeactivateControl(cH);
                        
        }    
                         
        // Set Allow Dropped Frames Check Box
        GetDialogItemAsControl(theDialog, CheckAllowDFID, &cH);
        SetControlValue(cH, AllowDF);
    
        // Set Quick Compress Check Box
        GetDialogItemAsControl(theDialog, CheckQuickCompressID, &cH);
        SetControlValue(cH, QuickCompress);
        
        DrawDialog(theDialog);    
    }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::InitEditControl(DialogPtr theDialog, short theItem, unsigned int theValue)
// Description: sets alignment and values of edit boxes 
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::InitEditControl(DialogPtr theDialog, short theItem, unsigned int theValue)
{
	Str255 	        tempStr;
	short           iType;
	Handle          theControl;
	Rect            iRect;
	GrafPtr         savePort;
		
	GetPort(&savePort);
	SetPort(GetDialogPort(theDialog));	
    GetDialogItem(theDialog, theItem, &iType, &theControl, &iRect);

    // Set the edit field to center text
    TESetAlignment(teCenter, GetDialogTextEditHandle(theDialog));
        
    //now put in the text
    NumToString(theValue, tempStr);
    SetDialogItemText(theControl, tempStr);
                
	SetPort(savePort); 
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Boolean dxlqtConfigDialog::ValidateCurrentEntry(DialogPtr theDialog, unsigned int lastEditID)
// Description: Makes sure values entered into dialog text fields fall within 
//              the corresponding slider's limits
// Args:    theDialog: pointer to our dialog
//          lastEditID: EditField's DITL number
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Boolean 
dxlqtConfigDialog::ValidateCurrentEntry(DialogPtr theDialog, DialogItemIndex textBoxID, int theValue)
{
    if(textBoxID != 0)
    {
        int             *valueToUpdate;
        int 			minValue;
        int 			maxValue;
    
        if(textBoxID == AutoKeyTextBoxID)
        {
            minValue = MIN_SENSITIVITY;
            maxValue = MAX_SENSITIVITY;
            valueToUpdate = &AutoKeyFrameThreshold;
        }
        else if(textBoxID == MinDistTextBoxID)
        {
            minValue = MIN_MINIMUM;
            maxValue = MAX_MINIMUM;
            valueToUpdate = &MinimumDistanceToKeyFrame;
        }
        else if(textBoxID == ForceKeyTextBoxID)
        {
            minValue = MIN_MAXIMUM;
            maxValue = MAX_MAXIMUM;
            valueToUpdate = &ForceKeyFrameEvery;
        }
        
    	//ok now we have the user entered value, so lets check
    	//to see if its within the valid range
        
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
            SelectDialogItemText(theDialog, textBoxID, 0, 100);
            return false;
        }
    }
    return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::UpdateSlider(DialogPtr theDialog, unsigned int lastEditID)
// Description: sets slider to already validated textbox value
// Args:    theDialog: ptr to dialog window
//          lastEditID: ID of slider's corresponding textbox
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::UpdateSlider(DialogPtr theDialog, DialogItemIndex textBoxID)
{
    ControlHandle cH;
    int theValue;

    switch(textBoxID)
    {
        case AutoKeyTextBoxID:
            GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
            SetControlValue(cH, AutoKeyFrameThreshold);
            break;

        case MinDistTextBoxID:
            GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
            SetControlValue(cH, MinimumDistanceToKeyFrame);
            
            // we have to make sure that the max kf value is not less than the min kf value

            // we update the max slider if necessary
		    GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
		    theValue = GetControlValue(cH);

            //we cant have a max keyframe value that is less than the min keyframe value
            if(MinimumDistanceToKeyFrame > theValue)
            {
    		    ForceKeyFrameEvery = MinimumDistanceToKeyFrame;
                SetControlValue(cH, ForceKeyFrameEvery);
                UpdateItemText(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);
                                                
                //set the focus back
                SelectDialogItemText(theDialog, MinDistTextBoxID, 0, 100);
                
                //refresh
                DrawDialog(theDialog);                
            }
            break;
            
        case ForceKeyTextBoxID:
            GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
            SetControlValue(cH, ForceKeyFrameEvery);
            
            //now update the min slider if necessary
    	    GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
            theValue = GetControlValue(cH);
                        
            //we cant have a max keyframe value that is less than the min keyframe value
            if(ForceKeyFrameEvery < theValue)
            {
    		    MinimumDistanceToKeyFrame = ForceKeyFrameEvery;
                SetControlValue(cH, MinimumDistanceToKeyFrame);
                UpdateItemText(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
                
                //set the focus back
                SelectDialogItemText(theDialog, ForceKeyTextBoxID, 0, 1000);
    
                //refresh
                DrawDialog(theDialog);                
            }

            break;
    }
}   

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::SetDefaults()
// Description: sets dialog member data to default vals, then
//              calls InitDialogControlSettings() to apply the changes
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

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::EmbedControls()
// Description: creates our control hierarchy by embedding all 
//              sliders/textfields in the AutoKeyFrameEnabled 
//              CheckBoxGroupBox
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::EmbedControls()
{
    ControlHandle cH, rCH;
    DialogPtr theDialog = m_theDialog;
    
    if(theDialog != nil)
    {
        GetDialogItemAsControl(theDialog, CheckAutoKeyID, &rCH);             
   
        GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
        EmbedControl(cH, rCH);
                       
        GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
        EmbedControl(cH, rCH);
      
        GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
        EmbedControl(cH, rCH);
       
        GetDialogItemAsControl(theDialog, AutoKeyTextBoxID, &cH);
        EmbedControl(cH, rCH);
                
        GetDialogItemAsControl(theDialog, MinDistTextBoxID, &cH);
        EmbedControl(cH, rCH);
        
        GetDialogItemAsControl(theDialog, ForceKeyTextBoxID, &cH);
        EmbedControl(cH, rCH);
    }
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//void dxlqtConfigDialog::updateSettings() 
//summary: updates member copy of settings
//Settings member should be changed to m_settings, no time now....
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::updateSettings()
{    
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
//int dxlqtConfigDialog::GetTextBoxValue(DialogItemIndex textBoxID)
//summary: gets text edit values (duh)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int
dxlqtConfigDialog::GetTextBoxValue(DialogItemIndex textBoxID)
{
    Str255          tempPStr;
    char            tempCStr[50];
    long            tempVal;
    Handle          tempHand;
    Rect            tempRect;
    short           iType;
    
    if(textBoxID < 6 || textBoxID > 8)
        return -1;
           
    GetDialogItem(m_theDialog, textBoxID, &iType, &tempHand, &tempRect);
    GetDialogItemText(tempHand, tempPStr);
    strcpy(tempCStr, PtoCStr(tempPStr));
   
    for(int i = 0; tempCStr[i] != '\0'; i++)
    {
        if(tempCStr[i] < 48 || tempCStr[i] > 57)
            return -1;
    }
    
   
    // need to get the string again, PtoCStr modifies the P str's contents
    GetDialogItemText(tempHand, tempPStr);
    StringToNum(tempPStr, &tempVal);

    return tempVal;
}
    

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::UpdateItemText(DialogPtr theDialog, short theItem, int theValue)
// Description: Takes textBox ID and a value, places the value in the text box and then 
//              selects/hilites the value
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void
dxlqtConfigDialog::UpdateItemText(DialogPtr theDialog, short theItem, int theValue)
{
    Str255 	        tempStr;
    Handle 	        tempHand;
    Rect            tempRect;
    short           iType;
    ControlHandle   cH;

    NumToString(theValue, tempStr);

    GetDialogItem(theDialog, theItem, &iType, &tempHand, &tempRect);
    GetDialogItemAsControl(theDialog, theItem, &cH);
    SetDialogItemText(tempHand, tempStr);

	TEHandle theTE = GetDialogTextEditHandle(theDialog);
	TEUpdate(&tempRect, theTE);

    // workaround for truncation of displayed textfield str
    SetKeyboardFocus(GetDialogWindow(theDialog), cH, kControlFocusNextPart);
    SelectDialogItemText(theDialog, theItem, 0, 100);
    
    DrawDialog(theDialog);
}
	
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// void dxlqtConfigDialog::DisplayDialog()
// Description: Initializes controls and manages the dialog box
// Note:        Added a second event selector that uses a minor kludge to prevent unnecessary
//              text field validations.  See m_bAutoTextFirstHit etc.
//                              
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
dxlqtConfigDialog::DisplayDialog()
{
    DialogPtr           theDialog;
    int                 theValue;
    		
    DialogItemIndex     itemHit;
    DialogItemIndex     lastItemHit = 0;    
	
    GrafPtr 		    savePort;
    short 			    saveResFile;
    
    // New for OS X
    ControlHandle       cH;
    short               dummyShort;
    ConstStr255Param    emptyStr = '\0';
        	
	saveResFile = CurResFile();
    myResFile = OpenComponentResFile((Component)(**internalGlob).regAccess->theCodec);
	
    // Open the dialog
    theDialog = GetNewDialog(dxlqtConfigDialogID, NULL, (WindowPtr)-1L);
	
	//save (this) so we can access it during the MyDialogFilter callback
	SetWRefCon(GetDialogWindow(theDialog), reinterpret_cast<long>(this));
	
	// storing member DialogPtr
	m_theDialog = theDialog;
	
	// Create our control hierarchy
	EmbedControls();
	
	// Prepare the dialog items
    InitDialogControlSettings();
	
    // Show the window and process its events
    ShowWindow(GetDialogWindow(theDialog));
    GetPort(&savePort);
    SetPort(GetDialogPort(theDialog));

    //show it once
    DrawDialog(theDialog);
	
    Finished = false;
    itemHit = 0;
	
    while(Finished != true)
    {
        // using default dialog filter - this eats all events
        // the only thing we're responsible for is filtering text field
        // input and monitoring/updating the sliders 
        ModalDialog(nil, &itemHit);
        m_theDialog = theDialog;             
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
    	    
    	    case AutoKeyTextBoxID:
    	        // did we just enter this text field?
    	        if(lastItemHit != AutoKeyTextBoxID)
    	        {
    	            m_bAutoTextFirstHit = true;
    	        }
    	        else
    	        {
    	            m_bAutoTextFirstHit = false;
    	        }
    	        break;
    	    case MinDistTextBoxID:
    	        if(lastItemHit != MinDistTextBoxID)
    	        {
    	            m_bMinTextFirstHit = true;
    	        }
    	        else
    	        {
    	            m_bMinTextFirstHit = false;
    	        }
    	        break;
    	        
    	    case ForceKeyTextBoxID:
    	        if(lastItemHit != ForceKeyTextBoxID)
    	        {
    	            m_bForceTextFirstHit = true;
    	        }
    	        else
    	        {
    	            m_bForceTextFirstHit = false;
    	        }
    	        break;
    	        
    	    case AutoKeyThresholdSliderID:
    		    GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
    		    AutoKeyFrameThreshold = GetControlValue(cH);
                UpdateItemText(theDialog, AutoKeyTextBoxID, AutoKeyFrameThreshold);
    		    break;
    
    	    case MinDistToKeySliderID:
    		    GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
    		    MinimumDistanceToKeyFrame = GetControlValue(cH);
                UpdateItemText(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
                
                //now update the max slider if necessary
    		    GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
    		    theValue = GetControlValue(cH);

                //we cant have a max keyframe value that is less than the min keyframe value
                if(MinimumDistanceToKeyFrame > theValue)
                {
        		    ForceKeyFrameEvery = MinimumDistanceToKeyFrame;
                    SetControlValue(cH, ForceKeyFrameEvery);
                    UpdateItemText(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);
                    
                }
                //set the focus back
                SelectDialogItemText(theDialog, MinDistTextBoxID, 0, 100);
                
                //refresh
                DrawDialog(theDialog);                
    		    break;
    		
    	    case ForceKeyFrameEverySliderID:
    		    GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
    		    ForceKeyFrameEvery = GetControlValue(cH);
                UpdateItemText(theDialog, ForceKeyTextBoxID, ForceKeyFrameEvery);
                
                //now update the min slider if necessary
    		    GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
    		    theValue = GetControlValue(cH);
                
                //we cant have a max keyframe value that is less than the min keyframe value
                if(ForceKeyFrameEvery < theValue)
                {
        		    MinimumDistanceToKeyFrame = ForceKeyFrameEvery;
                    SetControlValue(cH, MinimumDistanceToKeyFrame);
                    UpdateItemText(theDialog, MinDistTextBoxID, MinimumDistanceToKeyFrame);
                }
                    
                //set the focus back
                SelectDialogItemText(theDialog, ForceKeyTextBoxID, 0, 100);

                //refresh
                DrawDialog(theDialog);                
    		    break;
    
    	    case CheckAutoKeyID:
    		    GetDialogItemAsControl(theDialog, CheckAutoKeyID, &cH);
    		    theValue = GetControlValue(cH);
    		    theValue = !theValue;
    		    AutoKeyFrameEnabled = (Boolean)theValue;
    		    SetControlValue(cH, theValue);
    
    
                if(theValue)
                {
                    // auto key slider
                    GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
                    ActivateControl(cH);
    
                    // min dist slider
                    GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
                    ActivateControl(cH);
    
                    // force key slider
                    GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
                    ActivateControl(cH);
                    
                    // AutoKeyTextBox
                    GetDialogItemAsControl(theDialog, AutoKeyTextBoxID, &cH);
                    ActivateControl(cH);
                                                          
                    // MinDistTextBox
                    GetDialogItemAsControl(theDialog, MinDistTextBoxID, &cH);
                    ActivateControl(cH);
                    
                    // ForceKeyEveryTextBox
                    GetDialogItemAsControl(theDialog, ForceKeyTextBoxID, &cH);
                    ActivateControl(cH);
                }
                else
                {
                    // auto key slider
                    GetDialogItemAsControl(theDialog, AutoKeyThresholdSliderID, &cH);
                    DeactivateControl(cH);
    
                    // min dist slider
                    GetDialogItemAsControl(theDialog, MinDistToKeySliderID, &cH);
                    DeactivateControl(cH);
    
                    // force key slider
                    GetDialogItemAsControl(theDialog, ForceKeyFrameEverySliderID, &cH);
                    DeactivateControl(cH);
                                            
                    // AutoKeyTextBox
                    GetDialogItemAsControl(theDialog, AutoKeyTextBoxID, &cH);
                    DeactivateControl(cH);
                                                          
                    // MinDistTextBox
                    GetDialogItemAsControl(theDialog, MinDistTextBoxID, &cH);
                    DeactivateControl(cH);
                    
                    // ForceKeyEveryTextBox
                    GetDialogItemAsControl(theDialog, ForceKeyTextBoxID, &cH);
                    DeactivateControl(cH);
                }    
                
                DrawDialog(theDialog);    
    		    break;
        	
    	    case CheckAllowDFID:
    		    GetDialogItemAsControl(theDialog, CheckAllowDFID, &cH);
    		    theValue = GetControlValue(cH); 
    		    theValue = !theValue;
    	        AllowDF = (Boolean)theValue;			
    		    SetControlValue(cH, theValue);
    		    break;
    	
    	    case CheckQuickCompressID:
    		    GetDialogItemAsControl(theDialog, CheckQuickCompressID, &cH);
    		    theValue = GetControlValue(cH); 
    		    theValue = !theValue;
    		    QuickCompress = (Boolean)theValue;
    		    SetControlValue(cH, (unsigned char)theValue);
   		        break;    
   		    
   		    default:  // do nothing, the only way we can get here is a bad text box entry
   		         break;                            
	    } // end switch(itemHit)
        
        // This section is checking the text fields for input
        if(lastItemHit != itemHit)
        {
            // making sure the previous event was a text field entry
            if(lastItemHit >= AutoKeyTextBoxID && lastItemHit <= ForceKeyTextBoxID)
            {
                // making sure the user isn't still in the field typing
                if(!m_bAutoTextFirstHit || !m_bMinTextFirstHit || !m_bForceTextFirstHit)
                {
                    // if we're here it should be time to get the value out of the text field
                    switch(lastItemHit)
                    {
                        case AutoKeyTextBoxID:
                            theValue = GetTextBoxValue(AutoKeyTextBoxID);
                            if(theValue >= 0)
                            {
                                if(ValidateCurrentEntry(theDialog, AutoKeyTextBoxID, theValue))
                                {
                                    //AutoKeyFrameThreshold = theValue;
                                    UpdateSlider(theDialog, AutoKeyTextBoxID);
                                }
                            }
                            else
                            {
                                // we're not going to apply the settings if the user hits cancel
                                // so we complain if the user hit something else
                                if(itemHit != ButtonCancelID)
                                {
                                    StandardAlert (kAlertStopAlert, "\pInput Error", "\pEnter numbers only.",
            				    	                    nil, &dummyShort);
                                    SelectDialogItemText(theDialog, AutoKeyTextBoxID, 0, 100);       				  
                                }
                                
                                if(itemHit == ButtonOKID)
                                    Finished = false;
            				}
            				break;
            				            	        
            	        case MinDistTextBoxID:
            	            theValue = GetTextBoxValue(MinDistTextBoxID);
                            if(theValue >= 0)
                            {
                                if(ValidateCurrentEntry(theDialog, MinDistTextBoxID, theValue))
                                {
                                    //MinimumDistanceToKeyFrame = theValue;
                                    UpdateSlider(theDialog, MinDistTextBoxID);
                                }
                            }
                            else
                            {
                                if(itemHit != ButtonCancelID)
                                {
                                    StandardAlert (kAlertStopAlert, "\pInput Error", "\pEnter numbers only.",
            				    	                    nil, &dummyShort);
                                    SelectDialogItemText(theDialog, MinDistTextBoxID, 0, 100);       				  
            				    }
            				    
            				    if(itemHit == ButtonOKID)
            				        Finished = false;
            				}
            				break;
            				
                        case ForceKeyTextBoxID:
                            theValue = GetTextBoxValue(ForceKeyTextBoxID);
                            if(theValue >= 0)
                            {
                                if(ValidateCurrentEntry(theDialog, ForceKeyTextBoxID, theValue))
                                {
                                    //ForceKeyFrameEvery = theValue;
                                    UpdateSlider(theDialog, ForceKeyTextBoxID);                                    
                                }
                            }
                            else
                            {
                                if(itemHit != ButtonCancelID)
                                {
                                    StandardAlert (kAlertStopAlert, "\pInput Error", "\pEnter numbers only.",
            				    	                    nil, &dummyShort);
                                    SelectDialogItemText(theDialog, ForceKeyTextBoxID, 0, 100);
                                }
                                
                                if(itemHit == ButtonOKID)
                                    Finished = false;       				  
            				}
            				break;
            				
            		    default:
            		        break;
            	    }
                }
            }
        }
        
        DrawDialog(theDialog);
        lastItemHit = itemHit;
	} // while(Finished != true)
	
	// Close the dialog
	DisposeDialog(theDialog);
	
	SetPort(savePort);
	CloseComponentResFile(myResFile);
	UseResFile(saveResFile);
	
	// If OK button was clicked and the user was in a text field we NEED to update the settings again
	if(itemHit == ButtonOKID)
    	updateSettings();
	
}
