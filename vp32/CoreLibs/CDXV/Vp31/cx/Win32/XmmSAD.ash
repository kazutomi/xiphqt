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


;------------------------------------------------
XMMGetSADParams  STRUC
                    dd  6 dup (?)   ;6 pushed regs
                    dd  ?           ;return address
    NewDataPtr      dd  ?
    RefDataPtr      dd  ?
    PixelsPerLine   dd  ?
    ErrorSoFar      dd  ?
    BestSoFar       dd  ?
XMMGetSADParams  ENDS
;------------------------------------------------
