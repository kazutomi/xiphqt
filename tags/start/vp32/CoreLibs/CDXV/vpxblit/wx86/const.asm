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
;??? const.asm   
; read only data
;\***********************************************/ 
 
        .586
        .MODEL  flat, SYSCALL, os_dos
 
        .DATA 

WILK_DX_DATA SEGMENT PAGE PUBLIC USE32 'DATA' 

    ALIGN 32

    WK_CLEAR_UP_5BYTES      dq 00000000000ffffffh
    WK_CLEAR_7_3_BYTES      dq 000ffffff00ffffffh

    WK_johnsTable_MMX LABEL DWORD
        dq 0006060600000000h
        dq 0007070700010101h

        dq 0002020200040404h
        dq 0003030300050505h

        dq 0007070700010101h
        dq 0006060600000000h

        dq 0003030300050505h
        dq 0002020200040404h


    WK_RGB_MULFACTOR_555   dq 2000000820000008h
    WK_RB_MASK_555         dq 00f800f800f800f8h
    WK_G_MASK_555          dq 0000f8000000f800h

    WK_RGB_MULFACTOR_565   dq 2000000420000004h
    WK_RB_MASK_565         dq 00f800f800f800f8h
    WK_G_MASK_565          dq 0000fc000000fc00h

    WK_MASK_YY_MMX         dq 00ff00ff00ff00ffh
    WK_MASK_BYTE0          dq 00000000000000ffh

    ; John's 4x4 dither pattern 
    WK_johnsTable LABEL DWORD
         dd 7010600h
         dd 3050204h
         dd 6000701h
         dd 2040305h

WILK_DX_DATA ENDS 


PUBLIC WK_johnsTable_MMX
PUBLIC WK_johnsTable 

PUBLIC WK_RGB_MULFACTOR_555
PUBLIC WK_RB_MASK_555      
PUBLIC WK_G_MASK_555       

PUBLIC WK_RGB_MULFACTOR_565
PUBLIC WK_RB_MASK_565      
PUBLIC WK_G_MASK_565       

PUBLIC WK_MASK_YY_MMX
PUBLIC WK_MASK_BYTE0

PUBLIC WK_CLEAR_UP_5BYTES
PUBLIC WK_CLEAR_7_3_BYTES


;************************************************
        END
