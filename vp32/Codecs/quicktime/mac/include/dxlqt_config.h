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



#include <Appearance.h>
#include <Types.h>
#include <Controls.h>
#include <Events.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Sound.h>
#include <string.h>
#include <Resources.h>
#include <NumberFormatting.h>
#include <TextUtils.h>
#include <Files.h>
#include "duck_dxl.h"
#include "regentry.h"
#include "dxlqt_codec.h"
#include "GetSysFolder.h"
#include "common.h"
#include "type_aliases.h"

// constants for DITL vals
const int dxlqtConfigDialogID = 20000;
const int AutoKeyThresholdSliderID = 1;
const int MinDistToKeySliderID = 2;
const int ForceKeyFrameEverySliderID = 3;
const int CheckAutoKeyID = 12;
const int CheckAllowDFID = 4;
const int CheckQuickCompressID = 5;
const int AutoKeyTextBoxID = 6;
const int MinDistTextBoxID = 7;
const int ForceKeyTextBoxID = 8;
const int ButtonOKID = 9;
const int ButtonCancelID = 10;	
const int ButtonDefaultID = 11;


const int kDimmingBox = 21;



class dxlqtConfigDialog {
private:
    unsigned int 	    NoiseSensitivity;				// Preprocessor Setting in dialog
    unsigned int 	    KeyFrameDataTarget;
    unsigned int 	    AutoKeyFrameThreshold;			// %diff key frame threshold 
    unsigned int 	    MinimumDistanceToKeyFrame;		// min and max dist between key frames
    unsigned int 	    ForceKeyFrameEvery;	 		
    
    Boolean  		    AutoKeyFrameEnabled;			// Auto scene change detect
    Boolean 		    AllowDF;						// dropped frames
    Boolean 		    QuickCompress;
    Boolean             AllowWavelet;                               // not a user setting, internal
    Boolean	 	        applySettings;					// false unless user has hit OK to confirm settings
    Boolean 		    Finished;						// dialog finished?
    Boolean             DialogFirstRunFlag;
    Boolean             CodecFirstRunFlag;
	
    void                InitSettings();
    void 			    DisplayDialog();
    void			    SetDefaults();
    void 			    InitDialogControlSettings();
    void			    updateSettings();
    
    int				    GetTextBoxValue(int textBoxID);


//sjl
void 
InitEditControl(DialogPtr theDialog, short theItem, unsigned int theValue);
Boolean 
HandleKeyPress(DialogPtr theDialog, char theChar);
Boolean 
ValidateCurrentEntry(DialogPtr theDialog, unsigned int valueToUpdate);
Boolean 
HandleMouse(DialogPtr theDialog, Point pt, short modifiers);
static pascal Boolean 
MyDialogFilter(DialogPtr theDialog,EventRecord *ev,short *itemHit);
void
UpdateSlider(DialogPtr theDialog, unsigned int lastEditID);
void
UpdateItemText(DialogPtr theDialog, short theItem, int theValue);


//pascal	 void 
//DimEditLine(WindowPtr dwind, short dinum);


int lastControlEditID;
int kMaxEditLength;


public:
                        dxlqtConfigDialog(dxlqt_Globals glob);		
    short               myResFile;
    DialogPtr           theDialog;
    COMP_CONFIG* 	    Settings;						// internal Ptr to user settings
    dxlqt_Globals       internalGlob;
};