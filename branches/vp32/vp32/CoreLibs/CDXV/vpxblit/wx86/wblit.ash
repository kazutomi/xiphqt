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
;??? wblit.ash
; 
;\***********************************************/ 

;------------------------------------------------
x86_Params  STRUC
                    dd  6 dup (?)   ;6 pushed regs
                    dd  ?           ;return address
    dst             dd  ?
    scrnPitch       dd  ?
    buffConfig      dd  ?
x86_Params  ENDS
;------------------------------------------------

EXTERNDEF _WK_YforY:DWORD
EXTERNDEF _WK_UforBG:DWORD
EXTERNDEF _WK_VforRG:DWORD

EXTERNDEF _WK_YforY_MMX:DWORD
EXTERNDEF _WK_UforBG_MMX:DWORD
EXTERNDEF _WK_VforRG_MMX:DWORD

EXTERNDEF _WK_ClampTableR:DWORD
EXTERNDEF _WK_ClampTableG:DWORD
EXTERNDEF _WK_ClampTableB:DWORD

EXTERNDEF _WK_ClampTableR555:DWORD
EXTERNDEF _WK_ClampTableG555:DWORD
EXTERNDEF _WK_ClampTableB555:DWORD

EXTERNDEF _WK_ClampTableR565:DWORD
EXTERNDEF _WK_ClampTableG565:DWORD
EXTERNDEF _WK_ClampTableB565:DWORD

CLAMPCENTER EQU 256*4+128*4


EXTERNDEF WK_johnsTable_MMX:DWORD
EXTERNDEF WK_johnsTable:DWORD

EXTERNDEF WK_RGB_MULFACTOR_555:QWORD
EXTERNDEF WK_RB_MASK_555:QWORD
EXTERNDEF WK_G_MASK_555:QWORD

EXTERNDEF WK_RGB_MULFACTOR_565:QWORD
EXTERNDEF WK_RB_MASK_565:QWORD
EXTERNDEF WK_G_MASK_565:QWORD

EXTERNDEF WK_MASK_YY_MMX:DWORD
EXTERNDEF WK_MASK_BYTE0:DWORD

