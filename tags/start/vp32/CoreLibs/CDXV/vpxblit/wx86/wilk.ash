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
; 
;\***********************************************/ 

;;
;; YUV buffer configuration structure
;;
;------------------------------------------------
YUV_BUFFER_CONFIG  STRUC
    YWidth              dd ?
    YHeight             dd ?
    YStride             dd ?

    UVWidth             dd ?
    UVHeight            dd ?
    UVStride            dd ?

    YBuffer             dd ?
    UBuffer             dd ?
    VBuffer             dd ?

    uvStart             dd ?
    uvDstArea           dd ?
    uvUsedArea          dd ?
YUV_BUFFER_CONFIG  ENDS
;------------------------------------------------
