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


///////////////////////////////////////////////////////////////////////////
//
// dxlqt_codecdispatch.cpp
//
// Purpose: This is included by dxlqt_codec.cpp. It holds a list of macros
// which expand to case statements in a function (CDDispatch), which is 
// defined in the "ComponentDispatchHelper.c".
//
/////////////////////////////////////////////////////////////////////////


	ComponentSelectorOffset (6)

	ComponentRangeCount (3)
	ComponentRangeShift (8)
	ComponentRangeMask	(FF)

	ComponentRangeBegin (0)
		StdComponentCall (Target)
		ComponentError (Register)
		StdComponentCall (Version)
		StdComponentCall (CanDo)
		StdComponentCall (Close)
		StdComponentCall (Open)
	ComponentRangeEnd (0)
		
	ComponentRangeBegin (1)
		ComponentCall	  (GetCodecInfo)
#if COMP_BUILD
		ComponentCall	  (GetCompressionTime)
		ComponentCall	  (GetMaxCompressionSize)
		ComponentCall	  (PreCompress)
		ComponentCall	  (BandCompress)
#else 
		ComponentDelegate (GetCompressionTime)
		ComponentDelegate (GetMaxCompressionSize)
		ComponentDelegate (PreCompress)
		ComponentDelegate (BandCompress)
#endif
		ComponentDelegate (PreDecompress)
		ComponentDelegate (BandDecompress)
		ComponentDelegate (Busy)
#if	DECO_BUILD
		ComponentCall	  (GetCompressedImageSize)
#else 
		ComponentDelegate (GetCompressedImageSize)
#endif 
		ComponentDelegate (GetSimilarity)
		ComponentDelegate (TrimImage)
#if COMP_BUILD
		ComponentCall	  (RequestSettings)
		ComponentCall	  (GetSettings)
		ComponentCall	  (SetSettings)
#else 
		ComponentDelegate (RequestSettings)
		ComponentDelegate (GetSettings)
		ComponentDelegate (SetSettings)
#endif
		ComponentDelegate (Flush)
		ComponentDelegate (SetTimeCode)
		ComponentDelegate (IsImageDescriptionEquivalent)
		ComponentDelegate (NewMemory)
		ComponentDelegate (DisposeMemory)
		ComponentDelegate (HitTestData)
		ComponentDelegate (NewImageBufferMemory)
		ComponentDelegate (ExtractAndCombineFields)
		ComponentDelegate (GetMaxCompressionSizeWithSources)
		ComponentDelegate (SetTimeBase)
		ComponentDelegate (SourceChanged)
		ComponentDelegate (FlushLastFrame)
		ComponentDelegate (GetSettingsAsText)
		ComponentDelegate (GetParameterListHandle)
		ComponentDelegate (GetParameterList)
		ComponentDelegate (CreateStandardParameterDialog)
		ComponentDelegate (IsStandardParameterDialogEvent)
		ComponentDelegate (DismissStandardParameterDialog)
		ComponentDelegate (StandardParameterDialogDoAction)
		ComponentDelegate (NewImageGWorld)
		ComponentDelegate (DisposeImageGWorld)
	ComponentRangeEnd (1)

	ComponentRangeUnused (2)

	ComponentRangeBegin (3)
#if DECO_BUILD
		ComponentCall (Preflight)
		ComponentCall (Initialize)
		ComponentCall (BeginBand)
		ComponentCall (DrawBand)
		ComponentCall (EndBand)
		ComponentCall (QueueStarting)
		ComponentCall (QueueStopping)
#else
		ComponentDelegate (Preflight)
		ComponentDelegate (Initialize)
		ComponentDelegate (BeginBand)
		ComponentDelegate (DrawBand)
		ComponentDelegate (EndBand)
		ComponentDelegate (QueueStarting)
		ComponentDelegate (QueueStopping)
#endif 
	ComponentRangeEnd (3)
