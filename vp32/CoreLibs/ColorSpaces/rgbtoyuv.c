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

#include <assert.h>
#include "ColorConversions.h"
#include "lutbl.h"
#include "ccstr.h"

void ConvertRGBtoYUV(
    const unsigned char* const pucSourceR, const unsigned char* const pucSourceG, const unsigned char* const pucSourceB, 
    int width, int height, int rgb_step, int rgb_pitch,
    unsigned char* const pucDestY, unsigned char* const pucDestU, unsigned char* const pucDestV,  
    int uv_width_shift, int uv_height_shift,
    int y_step, int y_pitch,int uv_step,int uv_pitch
    )
{

    int i,j,k,l,m,n,o,p;
    int y_scaled,u_scaled,v_scaled;

    // remember r,g,b
    const unsigned char *r_ptr = pucSourceR;
    const unsigned char *g_ptr = pucSourceG;
    const unsigned char *b_ptr = pucSourceB;
    const unsigned char *r_ptr2 = pucSourceR;
    const unsigned char *g_ptr2 = pucSourceG;
    const unsigned char *b_ptr2 = pucSourceB;

    unsigned char* y_src = pucDestY;
    unsigned char* u_src = pucDestU;
    unsigned char* v_src = pucDestV;

    int *YRMultPtr = YRMult;  
    int *YGMultPtr = YGMult;  
    int *YBMultPtr = YBMult;  
    int *URMultPtr = URMult;  
    int *UGMultPtr = UGMult;  
    int *UBVRMultPtr = UBVRMult;
    int *VGMultPtr = VGMult;
    int *VBMultPtr = VBMult;

    int uv_width_mask = (0xFFFFFFFF>>(32-uv_width_shift));
    int uv_height_mask = (0xFFFFFFFF>>(32-uv_height_shift));

    int uv_this_row,uv_this_col;
    int uv_width_scale = 1 << uv_width_shift, uv_height_scale = 1 << uv_height_shift;
    int uv_sum_shift = uv_width_shift + uv_height_shift;
    

    int avg_round = 1 << ( uv_sum_shift - 1 );
    int r_sum,g_sum,b_sum;

    j=height+1;

    
    // do y rows
    while(--j)
    {
        // we have a uv on this row if all of the bits lower than shift are set
        uv_this_row = (uv_height_mask & j)==uv_height_mask;

        // do y columns
        i = width+1;
        k = 0;
        l = 0;
        m = 0;
        while(--i)
        {

            // calculate 3 multiply parts of rgb -> to y 
            y_scaled = YRMultPtr[r_ptr[k]]; // includes round value for shift factor
            y_scaled += YGMultPtr[g_ptr[k]];
            y_scaled += YBMultPtr[b_ptr[k]];
            y_src[l] = (unsigned char)(y_scaled >> ShiftFactor); 

            // we have a uv on this col if all of the bits lower than shift are set
            uv_this_col = (uv_width_mask & i)==uv_width_mask;

            // we now have a row and column on which we should calculate u and v values
            if(uv_this_row && uv_this_col)
            {
                r_sum = g_sum = b_sum = avg_round;

                r_ptr2 = r_ptr + k;
                g_ptr2 = g_ptr + k;
                b_ptr2 = b_ptr + k;

                // calculate r_sum, g_sum and b_sum for given u,v
                n = uv_height_scale + 1;
                while(--n)
                {
                    o = uv_width_scale+1;
                    p = 0;
                    while(--o)
                    {
                        r_sum += r_ptr2[p];
                        g_sum += g_ptr2[p];
                        b_sum += b_ptr2[p];

                        // step back one column
                        p -= rgb_step;
                    }

                    // step back one line
                    r_ptr2 -= rgb_pitch;
                    g_ptr2 -= rgb_pitch;
                    b_ptr2 -= rgb_pitch;

                }

                // calculate avg instead of just sum
                r_sum >>= uv_sum_shift;
                g_sum >>= uv_sum_shift;
                b_sum >>= uv_sum_shift;

                // convert rgb -> u,v
                u_scaled = URMultPtr[r_sum];
                v_scaled = UBVRMultPtr[r_sum];

                u_scaled += UGMultPtr[g_sum];
                v_scaled += VGMultPtr[g_sum];

                u_scaled += UBVRMultPtr[b_sum];
                v_scaled += VBMultPtr[b_sum];

                // convert from fixed point integer
                u_scaled >>= ShiftFactor;
                v_scaled >>= ShiftFactor;
                
                // save in result
                u_src[m] = (unsigned char) u_scaled;
                v_src[m] = (unsigned char) v_scaled;

                m+=uv_step;

            }

            k+=rgb_step;    // next rgb pixel
            l+=y_step;      // next y pixel

        }

        // we had uv's move to the next one
        if(uv_this_row)
        {
            u_src+=uv_pitch;
            v_src+=uv_pitch;
        }
        // next row
        r_ptr+=rgb_pitch;
        g_ptr+=rgb_pitch;
        b_ptr+=rgb_pitch;
        y_src+=y_pitch;
    }

    return;

}
