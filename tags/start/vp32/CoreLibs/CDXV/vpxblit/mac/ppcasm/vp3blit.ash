;//==========================================================================
;//
;//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
;//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
;//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
;//  PURPOSE.
;//
;//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
;//
;//--------------------------------------------------------------------------


;/***********************************************\
;??? vp3blit.ash
;- 
;\***********************************************/ 


;/*
;typedef struct {
;    int     YWidth;
;    int     YHeight;
;    int     YStride;
;
;    int     UVWidth;
;    int     UVHeight;
;    int     UVStride;
;
;    char *  YBuffer;
;    char *  UBuffer;
;    char *  VBuffer;
;
;    unsigned char     *uvStart;
;    int     uvDstArea;
;    int     uvUsedArea;
;} YUV_BUFFER_CONFIG;
;*/

;------------------------------------------------
	YUV_BUFFER_CONFIG:  RECORD
	YWidth:        ds.l 1
	YHeight:       ds.l 1
	YStride:       ds.l 1

	UVWidth:       ds.l 1
	UVHeight:      ds.l 1
	UVStride:      ds.l 1

	YBuffer:       ds.l 1
	UBuffer:       ds.l 1
	VBuffer:       ds.l 1

	temp1:         ds.l 1
	temp2:         ds.l 1
	temp3:         ds.l 1
	ENDR

;------------------------------------------------
	YUV_BUFFER_CONFIG_YV12:  RECORD
	YWidth:        ds.l 1
	YHeight:       ds.l 1
	YStride:       ds.l 1

	UVWidth:       ds.l 1
	UVHeight:      ds.l 1
	UVStride:      ds.l 1

	YBuffer:       ds.l 1
	UBuffer:       ds.l 1
	VBuffer:       ds.l 1

	uvStart:       ds.l 1
	uvDstArea:     ds.l 1
	uvUsedArea:    ds.l 1
	ENDR
