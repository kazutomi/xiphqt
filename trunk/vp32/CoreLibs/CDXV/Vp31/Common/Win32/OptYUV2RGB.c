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


/****************************************************************************
*
*   Module Title :     OptYUVtofromRGB
*
*   Description  :     Optimized YUV/RGB conversion functions
*
*
*****************************************************************************
*/

/****************************************************************************
*  Header Files
*****************************************************************************
*/
#ifdef PBDLL
#include "pbdll.h"
#endif
/****************************************************************************
*  Module constants.
*****************************************************************************
*/        

 
/****************************************************************************
*  Explicit imports
*****************************************************************************
*/

               
/****************************************************************************
*  Exported Global Variables
*****************************************************************************
*/

INT16 M205Vals[4] = {205, 205, 205, 205};
INT16 Minus128[4] = {-128,-128,-128,-128};
INT16 M73Vals[4] = {73, 73, 73, 73};
INT16 Minus16[4] =  {-16,-16,-16,-16};
/****************************************************************************
*  Forward References
*****************************************************************************
*/  

/****************************************************************************
*  Module Variables.
*****************************************************************************
*/  

/****************************************************************************
 * 
 *  ROUTINE       :     MmxYUVtoRGB
 *
 *  INPUTS        :     yblock, ublock, vblock
 *                           Blocks of Y U and V data.
 *                      uvoffset
 *                           Offset of UV quadrant
 *                      RGBPtr
 *                           RGB structure to write into.
 *                      ReconBuffer
 *                           Is the YUV source in reconstruction buffer format.
 *
 *  OUTPUTS       :     
 *.           
 *
 *  RETURNS       :     None.
 *
 *  FUNCTION      :     Converst one block from YUV to RGB
 *
 *  SPECIAL NOTES :     This functions still based upon the 
 *                      matrix Y = 6G + 3R + B. 
 *                             U = B - Y
 *                             V = R - Y
 *
 *
 *  ERRORS        :     None.
 *
 ****************************************************************************/

void MmxYUVtoRGB ( PB_INSTANCE * pbi,
				   YUV_BUFFER_ENTRY_PTR yblock,		// Y block to be decoded
				   YUV_BUFFER_ENTRY_PTR ublock,		// U block to be used
				   YUV_BUFFER_ENTRY_PTR vblock,		// V block to be used
			       int uvoffset,					// Offset to UV quadrant to be used
				   BGR_TYPE * RGBPtr,               // RGB bitmap data pointer
                   BOOL ReconBuffer )				// YUV buffer format
{
	INT32 n;
    INT32 RGB_YStep = (pbi->Configuration.VideoFrameWidth * 2);
    INT32 YStep;
    INT32 UVStep;
	UINT8 RVals[16];
	UINT8 BVals[16];
	UINT8 GVals[16];
	INT16 UFact[4];
	INT16 VFact[4];


	YUV_BUFFER_ENTRY_PTR YPtr;
	YUV_BUFFER_ENTRY_PTR YPtr2;
	YUV_BUFFER_ENTRY_PTR UPtr;
	YUV_BUFFER_ENTRY_PTR VPtr;

	BGR_TYPE * RGBPtr2 = RGBPtr + pbi->Configuration.VideoFrameWidth;

    // Set up starting values for YUV pointers
    YPtr = yblock;
    UPtr = ublock + uvoffset;
    VPtr = vblock + uvoffset;

    // Set the line step for the Y and UV planes and YPtr2
    if ( ReconBuffer )
    {
        YStep = (pbi->Configuration.YStride * 2);
        UVStep = pbi->Configuration.UVStride;
        YPtr2 = YPtr + pbi->Configuration.YStride;
    }
    else
    {
        YStep = (pbi->Configuration.VideoFrameWidth * 2);
        UVStep = (pbi->Configuration.VideoFrameWidth / 2);
        YPtr2 = YPtr + pbi->Configuration.VideoFrameWidth;
    }

__asm
    {
    	pxor        mm6, mm6					; Blank mmx6
    }

    for ( n = 0; n < BLOCK_HEIGHT_WIDTH/2; n++ )
    {
__asm
        {
            // Create the VFactor data ((V - 128) * 1.6) => (* 1.6 approximated as (*205 / 128))
            // and        UFactor data ((U - 128) * 2)
            mov         ebx,dword ptr [VPtr]        ; Load 4 V values
            mov         ecx,dword ptr [UPtr]        ; Load 4 U values
            movd        mm0,dword ptr [ebx] 
            movd        mm1,dword ptr [ecx] 

            punpcklbw   mm0, mm6                    ; Unpack V to words
            movq        mm3, dword ptr[Minus128]    ; Load -128 into each word of mm3.
            punpcklbw   mm1, mm6                    ; Unpack U to words
            movq        mm4, dword ptr [M205Vals]   ; 205 in each word of mm4

            paddsw      mm0, mm3                    ; Add -128 to V values
            paddsw      mm1, mm3                    ; Add -128 to U values
            pmullw      mm0, mm4                    ; Multiply by 205 (safe in 16 bit signed space)
            paddsw      mm1, mm1                    ; =((U-128)*2)
            psraw       mm0, 7                      ; Divide by 128 maintaining sign
            movq        dword ptr [UFact], mm1      ; Save back to U factors array.
            movq        dword ptr [VFact], mm0      ; Save back to V factors array.

			// Load and unpack the Y data
            mov         eax,dword ptr [YPtr]        ; Load 8 Y values in the first row
            movq        mm2,dword ptr [eax] 
            movq        mm3, mm2                    ; Take a copy     
		    punpcklbw   mm2, mm6					; unpack low four bytes of Y
		    punpckhbw   mm3, mm6					; unpack high four bytes of Y

            // Scale Y back to full range Y = (Y-16)*73/64
            movq        mm1, dword ptr[Minus16]     ; Load -16 into each word of mm1
            movq        mm0, dword ptr[M73Vals]     ; Load 73 into each word of mm0
            paddsw      mm2, mm1                    ; Add -16 to Y values
            paddsw      mm3, mm1                    ; Add -16 to Y values
            pmullw      mm2, mm0                    ; Multiply by 73 (safe in 16 bit signed space)
            pmullw      mm3, mm0                    ; Multiply by 73 (safe in 16 bit signed space)
            psraw       mm2, 6                      ; Divide by 64 maintaining sign
            psraw       mm3, 6                      ; Divide by 64 maintaining sign

            // Calculation of 8 R values
            movq        mm0,dword ptr [VFact]       ; Move 4 V factors into a register
            movq        mm1, mm0                    ; Take a copy        
            punpcklwd   mm0, mm0                    ; Unpack and duplicate
            punpckhwd   mm1, mm1                    ; Unpack and duplicate

            movq        mm4, mm2                    ; Take a copy of packed Y data
            movq        mm5, mm3                    
			paddw       mm4, mm0                    ; Add low V factors to low Y values
            paddw       mm5, mm1                    ; Add high V factors to high Y values

            packuswb    mm4, mm5                    ; Pack and saturate the data (now R data)
            movq        dword ptr [RVals], mm4      ; Save it back (keep R data in mm4 for use later)

            // Calculation of 8 B values
            movq        mm0,dword ptr [UFact]       ; Move 4 U factors into a register
            movq        mm1, mm0                    ; Take a copy        
            punpcklwd   mm0, mm0                    ; Unpack and duplicate
            punpckhwd   mm1, mm1                    ; Unpack and duplicate

            movq        mm7, mm2                    ; Take a copy of packed Y data
            movq        mm5, mm3                    
			paddw       mm7, mm0                    ; Add low U factors to low Y values
            paddw       mm5, mm1                    ; Add high U factors to high Y values

            packuswb    mm7, mm5                    ; Pack and saturate the data (now R data)
            movq        dword ptr [BVals], mm7      ; Save it back

            // Calculate G values ((Y*10) - 3R - B) / 6
            // X/6 is aproximated as ((2.625 X) /16)
            // Calucaltions at word resolution (low order words then high order)
            // finally packed down to saturated U
		    movq        mm5, mm2					; Copy Low four Y values
            movq        mm0, mm4                    ; Copy R bytes
            psllw       mm2, 3                      ; Multiply Y by 8
            paddw       mm5, mm5                    ; Add Y to itself = 2Y
            punpcklbw   mm0, mm6                    ; Unpack low R bytes to words
            paddw       mm2, mm5                    ; Add 2Y to 8Y = 10Y
            movq        mm5, mm7                    ; Copy B
            psubsw      mm2, mm0                    ; 10Y - R
            punpcklbw   mm5, mm6                    ; Unpack low B
            paddw       mm0, mm0                    ; 2R
            psubsw      mm2, mm5                    ; 10Y - R - B
            psubsw      mm2, mm0                    ; 10Y - 3R - B
	
            movq        mm1, mm2                    ; Copy data
            paddw       mm1, mm1                    ; Add to itself to give 2X
            psraw       mm2, 1                      ; Devide by 2 to give 0.5X
            paddw       mm1, mm2                    ; Add in to give 2.5X
            psraw       mm2, 2                      ; Devide by 4 to give 0.125X
            paddw       mm1, mm2                    ; Add in to give 2.625X
            psraw       mm1, 4                      ; Divide by 16 

			movq        mm5, mm3					; Copy four high Y values
            movq        mm0, mm4                    ; Copy R bytes
            psllw       mm5, 3                      ; Multiply Y by 8
            punpckhbw   mm0, mm6                    ; Unpack low R bytes to words
            paddw       mm3, mm3                    ; Add Y to itself = 2Y
            paddw       mm5, mm3                    ; Add 2Y to 8Y = 10Y
            movq        mm2, mm7                    ; Copy B
            psubsw      mm5, mm0                    ; 10Y - R
            punpckhbw   mm2, mm6                    ; Unpack low B
            paddw       mm0, mm0                    ; 2R
            psubsw      mm5, mm2                    ; 10Y - R - B
            psubsw      mm5, mm0                    ; 10Y - 3R - B

            // finish off the G processing and store
            movq        mm2, mm5                    ; Copy data
            paddw       mm5, mm5                    ; Add to itself to give 2X
            psraw       mm2, 1                      ; Devide by 2 to give 0.5X
            paddw       mm5, mm2                    ; Add in to give 2.5X
            psraw       mm2, 2                      ; Devide by 4 to give 0.125X
            paddw       mm5, mm2                    ; Add in to give 2.625X
            psraw       mm5, 4                      ; Divide by 16 
            packuswb    mm1, mm5                    ; Pack and saturate the data (now g data)
            movq        dword ptr [GVals], mm1      ; Save data back to ram

			// Load and unpack the second row of Y data and
            // finish of the G processing for the previous row (indented).
            mov         eax,dword ptr [YPtr2]       ; Load 8 Y values in the first row
            movq        mm2,dword ptr [eax] 
            movq        mm3, mm2                    ; Take a copy     
		    punpcklbw   mm2, mm6					; unpack low four bytes of Y
		    punpckhbw   mm3, mm6					; unpack high four bytes of Y

            // Scale Y back to full range Y = (Y-16)*73/64
            movq        mm1, dword ptr[Minus16]     ; Load -16 into each word of mm1
            movq        mm0, dword ptr[M73Vals]     ; Load 73 into each word of mm0
            paddsw      mm2, mm1                    ; Add -16 to Y values
            paddsw      mm3, mm1                    ; Add -16 to Y values
            pmullw      mm2, mm0                    ; Multiply by 73 (safe in 16 bit signed space)
            pmullw      mm3, mm0                    ; Multiply by 73 (safe in 16 bit signed space)
            psraw       mm2, 6                      ; Divide by 64 maintaining sign
            psraw       mm3, 6                      ; Divide by 64 maintaining sign

            // Calculation of 8 R values
            movq        mm0, dword ptr [VFact]      ; Move 4 V factors into a register
            movq        mm1, mm0                    ; Take a copy        
            punpcklwd   mm0, mm0                    ; Unpack and duplicate
            punpckhwd   mm1, mm1                    ; Unpack and duplicate

            movq        mm4, mm2                    ; Take a copy of packed Y data
            movq        mm5, mm3                    
			paddw       mm4, mm0                    ; Add low V factors to low Y values
            paddw       mm5, mm1                    ; Add high V factors to high Y values

            packuswb    mm4, mm5                    ; Pack and saturate the data (now R data)
            movq        dword ptr [RVals + 8], mm4  ; Save it back (keep R data in mm4 for use later)

            // Calculation of 8 B values
            movq        mm0,dword ptr [UFact]       ; Move 4 U factors into a register
            movq        mm1, mm0                    ; Take a copy        
            punpcklwd   mm0, mm0                    ; Unpack and duplicate
            punpckhwd   mm1, mm1                    ; Unpack and duplicate

            movq        mm7, mm2                    ; Take a copy of packed Y data
            movq        mm5, mm3                    
			paddw       mm7, mm0                    ; Add low U factors to low Y values
            paddw       mm5, mm1                    ; Add high U factors to high Y values

            packuswb    mm7, mm5                    ; Pack and saturate the B data
            movq        dword ptr [BVals + 8], mm7  ; Save it back

            // Calculate G values ((Y*10) - 3R - B) / 6
            // X/6 is aproximated as ((2.625 X) /16)
            // Calucaltions at word resolution (low order words then high order)
            // finally packed down to saturated U
		    movq        mm5, mm2					; Copy Low four Y values
            movq        mm0, mm4                    ; Copy R bytes
            psllw       mm2, 3                      ; Multiply Y by 8
            paddw       mm5, mm5                    ; Add Y to itself = 2Y
            punpcklbw   mm0, mm6                    ; Unpack low R bytes to words
            paddw       mm2, mm5                    ; Add 2Y to 8Y = 10Y
            movq        mm5, mm7                    ; Copy B
            psubsw      mm2, mm0                    ; 10Y - R
            punpcklbw   mm5, mm6                    ; Unpack low B
            paddw       mm0, mm0                    ; 2R
            psubsw      mm2, mm5                    ; 10Y - R - B
            psubsw      mm2, mm0                    ; 10Y - 3R - B

            movq        mm1, mm2                    ; Copy data
            paddw       mm1, mm1                    ; Add to itself to give 2X
            psraw       mm2, 1                      ; Devide by 2 to give 0.5X
            paddw       mm1, mm2                    ; Add in to give 2.5X
            psraw       mm2, 2                      ; Devide by 4 to give 0.125X
            paddw       mm1, mm2                    ; Add in to give 2.625X
            psraw       mm1, 4                      ; Divide by 16 

		    movq        mm5, mm3					; Copy four high Y values
            movq        mm0, mm4                    ; Copy R bytes
            psllw       mm5, 3                      ; Multiply Y by 8
            punpckhbw   mm0, mm6                    ; Unpack low R bytes to words
            paddw       mm3, mm3                    ; Add Y to itself = 2Y
            paddw       mm5, mm3                    ; Add 2Y to 8Y = 10Y
            movq        mm2, mm7                    ; Copy B
            psubsw      mm5, mm0                    ; 10Y - R
            punpckhbw   mm2, mm6                    ; Unpack low B
            paddw       mm0, mm0                    ; 2R
            psubsw      mm5, mm2                    ; 10Y - R - B
            psubsw      mm5, mm0                    ; 10Y - 3R - B

            // finish off the G processing and store
            movq        mm2, mm5                    ; Copy data
            paddw       mm5, mm5                    ; Add to itself to give 2X
            psraw       mm2, 1                      ; Devide by 2 to give 0.5X
            paddw       mm5, mm2                    ; Add in to give 2.5X
            psraw       mm2, 2                      ; Devide by 4 to give 0.125X
            paddw       mm5, mm2                    ; Add in to give 2.625X
            psraw       mm5, 4                      ; Divide by 16 
            packuswb    mm1, mm5                    ; Pack and saturate the data (now g data)
            movq        dword ptr [GVals + 8], mm1  ; Save data back to ram
        }   

        // Copy the results into the RGB structure
		RGBPtr[0].Red	= RVals[0];
		RGBPtr[0].Green	= GVals[0];
		RGBPtr[0].Blue	= BVals[0];
		RGBPtr[1].Red	= RVals[1];
		RGBPtr[1].Green	= GVals[1];
		RGBPtr[1].Blue	= BVals[1];
		RGBPtr[2].Red	= RVals[2];
		RGBPtr[2].Green	= GVals[2];
		RGBPtr[2].Blue	= BVals[2];
		RGBPtr[3].Red	= RVals[3];
		RGBPtr[3].Green	= GVals[3];
		RGBPtr[3].Blue	= BVals[3];
		RGBPtr[4].Red	= RVals[4];
		RGBPtr[4].Green	= GVals[4];
		RGBPtr[4].Blue	= BVals[4];
		RGBPtr[5].Red	= RVals[5];
		RGBPtr[5].Green	= GVals[5];
		RGBPtr[5].Blue	= BVals[5];
		RGBPtr[6].Red	= RVals[6];
		RGBPtr[6].Green	= GVals[6];
		RGBPtr[6].Blue	= BVals[6];
		RGBPtr[7].Red	= RVals[7];
		RGBPtr[7].Green	= GVals[7];
		RGBPtr[7].Blue	= BVals[7];

		RGBPtr2[0].Red	= RVals[8];
		RGBPtr2[0].Green = GVals[8];
		RGBPtr2[0].Blue	= BVals[8];
		RGBPtr2[1].Red	= RVals[9];
		RGBPtr2[1].Green = GVals[9];
		RGBPtr2[1].Blue	= BVals[9];
		RGBPtr2[2].Red	= RVals[10];
		RGBPtr2[2].Green = GVals[10];
		RGBPtr2[2].Blue	= BVals[10];
		RGBPtr2[3].Red	= RVals[11];
		RGBPtr2[3].Green = GVals[11];
		RGBPtr2[3].Blue	= BVals[11];
		RGBPtr2[4].Red	= RVals[12];
		RGBPtr2[4].Green = GVals[12];
		RGBPtr2[4].Blue	= BVals[12];
		RGBPtr2[5].Red	= RVals[13];
		RGBPtr2[5].Green = GVals[13];
		RGBPtr2[5].Blue	= BVals[13];
		RGBPtr2[6].Red	= RVals[14];
		RGBPtr2[6].Green = GVals[14];
		RGBPtr2[6].Blue	= BVals[14];
		RGBPtr2[7].Red	= RVals[15];
		RGBPtr2[7].Green = GVals[15];
		RGBPtr2[7].Blue	= BVals[15];

		// Increment the various pointers
 		YPtr += YStep;
		YPtr2 += YStep;
		UPtr += UVStep;
		VPtr += UVStep;
		RGBPtr += RGB_YStep;
		RGBPtr2 += RGB_YStep;
    }
             
__asm   
    {
        emms									; Clear the MMX state.
    }
}
