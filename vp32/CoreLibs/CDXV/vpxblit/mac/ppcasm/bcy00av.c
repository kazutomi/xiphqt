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


#include "vfw_pb_interface.h"

void bcy00av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
{

	/* only use this loop if the dest starts on a 16byte boundary and the pitch is a multiple of 16 */
	if((!((unsigned long)_ptrScreen & 0xf)) && !(thisPitch & 0xf))
	{
		/* destination is 16 byte aligned */
		/* NOTE: compiler uses r12 for vrsave */	
		asm
		{
			
		    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
		    xor         r30,r30,r30             					//;clear hIndex
			
		    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)

		    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)

		    lwz         r9, YUV_BUFFER_CONFIG.YWidth(r5)

		    lwz         r31,YUV_BUFFER_CONFIG.YHeight(r5)

		    lwz         r29,YUV_BUFFER_CONFIG.YStride(r5)

		    lwz         r28,YUV_BUFFER_CONFIG.UVStride(r5)

		h_loop:
			xor			r10,r10,r10									//y width index
			xor			r27,r27,r27									//uv width index

			xor			r26,r26,r26									//store index


		w_loop:
			lvx			v10,r6,r10									//16 y's
			addi		r10,r10,16
			
			lvsl		v8,r7,r27									//u alignment vector
	cmpw 		r9,r10

			lvsl		v9,r8,r27									//v alignment vector

			lvx			v11,r7,r27									//only interested in 8 u's
			
			lvx			v12,r8,r27									//only interested in 8 v's
			vperm		v11,v11,v11,v8
			addi		r27,r27,8

			vperm		v12,v12,v12,v9
	
			vmrghb		v0,v11,v12									//interleave u's and v's

			vmrghb		v1,v10,v0
			
			vmrglb		v2,v10,v0

			stvx 		v1,r3,r26
			addi		r26,r26,16
			
			stvx 		v2,r3,r26
			addi		r26,r26,16
			bne      	w_loop                   

		    slwi        r24,r30,31
		    addi        r30,r30,1               					//;inc line ptr

		    cmpw        r30,r31

		    srawi       r24,r24,31              					//;generate mask
		    subf        r6,r29,r6               					//;next line of y's

		    and         r24,r24,r28             					//;will be 0 or UVStride
		    add         r3,r3,r4                					//;next scan line
		    

		    subf        r7,r24,r7               
		    subf        r8,r24,r8
		    bne         h_loop
		} 
	}
	else
	{
		/* destination is NOT 16 byte aligned */
		/* NOTE: compiler uses r12 for vrsave */	
		asm
		{
			
		    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
		    xor         r30,r30,r30             					//;clear hIndex
			
		    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)

		    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)

		    lwz         r9, YUV_BUFFER_CONFIG.YWidth(r5)

		    lwz         r31,YUV_BUFFER_CONFIG.YHeight(r5)

		    lwz         r29,YUV_BUFFER_CONFIG.YStride(r5)

		    lwz         r28,YUV_BUFFER_CONFIG.UVStride(r5)

		h_loop2:
			xor			r10,r10,r10									//y width index
			xor			r27,r27,r27									//uv width index

			xor			r26,r26,r26									//store index


		w_loop2:
			lvx			v10,r6,r10									//16 y's
			addi		r10,r10,16
			
			lvsl		v8,r7,r27									//u alignment vector
	cmpw 		r9,r10

			lvsl		v9,r8,r27									//v alignment vector

			lvx			v11,r7,r27									//only interested in 8 u's
			
			lvx			v12,r8,r27									//only interested in 8 v's
			vperm		v11,v11,v11,v8
			addi		r27,r27,8

			vperm		v12,v12,v12,v9
	
			vmrghb		v0,v11,v12									//interleave u's and v's

			vmrghb		v1,v10,v0
			
			lvsl		v8,r3,r26
			vmrglb		v2,v10,v0
			
			vperm		v1,v1,v1,v8

			stvewx 		v1,r3,r26
			addi		r26,r26,4
			
			stvewx 		v1,r3,r26
			addi		r26,r26,4

			stvewx 		v1,r3,r26
			addi		r26,r26,4

			stvewx 		v1,r3,r26
			addi		r26,r26,4

			lvsl		v8,r3,r26

			vperm		v2,v2,v2,v8

			stvewx 		v2,r3,r26
			addi		r26,r26,4

			stvewx 		v2,r3,r26
			addi		r26,r26,4

			stvewx 		v2,r3,r26
			addi		r26,r26,4

			stvewx 		v2,r3,r26
			addi		r26,r26,4
			bne      	w_loop2                   

		    slwi        r24,r30,31
		    addi        r30,r30,1               					//;inc line ptr

		    cmpw        r30,r31

		    srawi       r24,r24,31              					//;generate mask
		    subf        r6,r29,r6               					//;next line of y's

		    and         r24,r24,r28             					//;will be 0 or UVStride
		    add         r3,r3,r4                					//;next scan line
		    

		    subf        r7,r24,r7               
		    subf        r8,r24,r8
		    bne         h_loop2
		} 
	}
}

