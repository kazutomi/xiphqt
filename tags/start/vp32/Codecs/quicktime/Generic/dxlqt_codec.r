#define SystemSevenOrLater 1
#define UseExtendedThingResource 1

#include "ConditionalMacros.r"
#include "MacTypes.r"
#include "Components.r"
#include "ImageCodec.r"
#include "Common.h"

#if TARGET_OS_MAC
#define Target_PlatformType      platformPowerPC
#else 
#define Target_PlatformType      platformWin32
#endif 

#define	kCodecVendor		'On2'
#define	kCodecFormatName	"VP31"
#define	kCodecFormatNameC	"On2 VP3 Video 3.2"

#define kDecoFlags		    ( codecInfoDoes32 | codecInfoDoes16 )
#define	kVP31FormatType	    'VP31'
#define kCompFlags          ( codecInfoDoes32 | codecInfoDoesTemporal | codecInfoDoesRateConstrain)
#define kFormatFlags		( codecInfoDepth32 )

resource 'STR ' (130) {
	"VP32 Video Player"
};


//*************************************************************************
//	Build switches
//*************************************************************************
#define COMP_BUILD_R            1       //set this to 0 for dx only
#define COMP_PRO_BUILD_R        0
#define COMP_PROEVAL_BUILD_R    0

//*************************************************************************
//	Version INFO
//*************************************************************************
#define SHORTVERSIONSTRING "3.2.1.3"
#define LONGVERSIONSTRING "On2 VP3 Video Codec 3.2.1.3\nOn2 Technologies, The Duck Corporation 2001"

 resource 'vers' (1, purgeable) {
    0x03, 0x20, final, 0x03, verUS,
    SHORTVERSIONSTRING,
    LONGVERSIONSTRING
 };
 resource 'vers' (2, purgeable) {
    0x03, 0x20, final, 0x03, verUS,
    SHORTVERSIONSTRING,
    "(for QuickTime 4.0 or greater)"
 };



#if COMP_BUILD_R
//*************************************************************************
//	VP32 COMPRESSOR INFO
//*************************************************************************
resource 'cdci' (resid_Comp) {
	kCodecFormatNameC,		    //	Format Name
	Codec_Version,				//	Version
	Codec_Revision,				//	Rev Level
	kCodecVendor,			    //	Vendor
	kDecoFlags,			        //	Decompress Flags
	kCompFlags,					//	Compress Flags
	kFormatFlags,			    //	Format Flags
	100,						//	Compression Accuracy
	100,						//	Decompression Accuracy
	200,						//	Compression Speed
	200,						//	Decompression Speed
	100,						//	Compression Level
	0,							//	Reserved
	2,							//	Min Height
	2,							//	Min Width
	0,							//	Decompress Pipeline Latency
	0,							//	Compress Pipeline Latency
	0							//	Private Data
};

resource 'thng' (resid_Comp) {
	compressorComponentType,	// Type
	kVP31FormatType,		    // Sub-Type
	kCodecVendor,			    // Vendor
	kCompFlags,			        // Flags
	0,							// Mask
	'cdec', resid_Comp,			// ID
	'STR ',	resid_CompName,		// Name
	'STR ',	resid_CompInfo,		// Info
	0,	0,						// Icon
	Codec_Version,
	componentHasMultiplePlatforms + 
		componentDoAutoVersion,
	0,
	{
#if TARGET_OS_MAC
		kCompFlags, 
		'cdek',
		resid_Comp,
		Target_PlatformType,
#else	
		kCompFlags, 
		'dlle',
		resid_Comp,
		Target_PlatformType
#endif
	};
};

resource 'STR ' (resid_CompInfo) {
	"Compresses images into the On2 VP3 format."
};
resource 'STR ' (resid_CompName) {
	"On2 VP3 Video Compressor"
};

#if	!TARGET_OS_MAC
	resource 'dlle' (resid_Comp) {
		"CDComponentDispatch"
	};
#endif

#define codeResId resid_Comp

#else

#define codeResId resid_Deco

#endif


//*************************************************************************
//	VP32 DECOMPRESSOR INFO
//*************************************************************************
resource 'cdci' (resid_Deco) 
{
	kCodecFormatName,		    //	Format Name
	Codec_Version,				//	Version
	Codec_Revision,				//	Rev Level
	kCodecVendor,			    //	Vendor
	kDecoFlags,			        //	Decompress Flags
	kCompFlags,					//	Compress Flags
	kFormatFlags,			    //	Format Flags
	100,						//	Compression Accuracy
	100,						//	Decompression Accuracy
	200,						//	Compression Speed
	200,						//	Decompression Speed
	100,						//	Compression Level
	0,							//	Reserved
	2,							//	Min Height
	2,							//	Min Width
	0,							//	Decompress Pipeline Latency
	0,							//	Compress Pipeline Latency
	0							//	Private Data
};

resource 'thng' (resid_Deco) 
{
	decompressorComponentType,	// Type
	kVP31FormatType,		    // Sub-Type
	kCodecVendor,			    // Vendor
	kDecoFlags,			        // Flags
	0,							// Mask
	'cdec', resid_Deco,			// ID
	'STR ',	resid_DecoName,		// Name
	'STR ',	resid_DecoInfo,		// Info
	0,	0,						// Icon
	Codec_Version,

	componentHasMultiplePlatforms + 
		componentDoAutoVersion,
	0,
	{
#if TARGET_OS_MAC
		kDecoFlags, 
		'cdek',
		codeResId,
		Target_PlatformType
#else	
		kDecoFlags, 
		'dlle',
		codeResId,
		Target_PlatformType
#endif
	};
};

resource 'STR ' (resid_DecoName) {
	"On2 VP3 Video Playback"
};

resource 'STR ' (resid_DecoInfo) {
	"Decompresses images stored in the On2 VP3 format."
};


#if	!TARGET_OS_MAC
	resource 'dlle' (resid_Deco) {
		"CDComponentDispatch"
	};
#endif
