#include <ImageCodec.h>
#include "duck_dxl.h"

#include "dxlqt_helper.h"
#include "regentry.h"

#include "dxlqt_codec.h"

#if COMP_BUILD									// compressor variables 
#ifdef VP3_COMPRESS
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void 
getCompConfigDefaultSettings(COMP_CONFIG *CompConfig)
{
	CompConfig->FrameSize						= 0;
	CompConfig->TargetBitRate					= 300;
	CompConfig->FrameRate 						= 25;
	CompConfig->KeyFrameFrequency 				= 120;
	CompConfig->KeyFrameDataTarget 				= 110;      
	CompConfig->Quality 						= 56;
	CompConfig->AllowDF 						= TRUE;
	CompConfig->QuickCompress					= TRUE;    
	CompConfig->AutoKeyFrameEnabled				= TRUE;    
	CompConfig->AutoKeyFrameThreshold			= 80;
	CompConfig->MinimumDistanceToKeyFrame		= 8;
	CompConfig->ForceKeyFrameEvery 				= 120;
    CompConfig->NoiseSensitivity				= 1;        
}
#endif
#endif
