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
CConvParams  STRUC
                    dd  6 dup (?)   ;6 pushed regs
                    dd  ?           ;return address
    RGBABuffer      dd  ?
    ImageWidth      dd  ?
    ImageHeight     dd  ?
    YBuffer         dd  ?
    UBuffer         dd  ?
    VBuffer         dd  ?
CConvParams  ENDS
;------------------------------------------------

YVYUConvParams  STRUC
                    dd  6 dup (?)   ;6 pushed regs
                    dd  ?           ;return address
    YVYUBuffer      dd  ?
    ImageWidth      dd  ?
    ImageHeight     dd  ?
    YBuffer         dd  ?
    UBuffer         dd  ?
    VBuffer         dd  ?
YVYUConvParams  ENDS

;------------------------------------------------
