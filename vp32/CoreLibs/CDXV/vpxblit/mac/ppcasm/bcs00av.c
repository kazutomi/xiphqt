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

extern vector signed short vp3_vConst1;
extern vector signed short vp3_ditherPats_av[4];

extern void bcs00(unsigned char *dst, int scrnPitch, YUV_BUFFER_CONFIG *buffConfig); 

void bcs00av(unsigned char *_ptrScreen, int thisPitch, YUV_BUFFER_CONFIG *src)
{

	/* only use this loop if the dest starts on a 16byte boundary and the pitch is a multiple of 16 */
	if((!((unsigned long)_ptrScreen & 0xf)) && !(thisPitch & 0xf))
	{
		/* destination is 16 byte aligned */
		/* NOTE: compiler uses r25 for vrsave */	
		asm
		{
  		    lwz         r24,vp3_ditherPats_av
		    xor			r0,r0,r0

		    lwz         r26,vp3_vConst1
		    xor         r30,r30,r30             					//;clear hIndex
			vspltisb 	v30,15
		    
			lvx			v0,r26,r30
			vxor		v7,v7,v7
			vspltisb 	v31,2
			
		    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
			vsplth		v1,v0,1										//1.596 * 32768
			
		    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)
			vsplth		v2,v0,2										//-0.813 * 32768

		    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)
			vsplth		v3,v0,3										//-0.392 * 32768

		    lwz         r9, YUV_BUFFER_CONFIG.YWidth(r5)
			vsplth		v4,v0,4										//2.017 * 32768

		    lwz         r31,YUV_BUFFER_CONFIG.YHeight(r5)
			vsplth		v5,v0,5										//16

		    lwz         r29,YUV_BUFFER_CONFIG.YStride(r5)
			vsplth		v6,v0,6										//128

		    lwz         r28,YUV_BUFFER_CONFIG.UVStride(r5)
			vsplth		v0,v0,0										//1.164 * 32768

		h_loop:
			lvx			v29,r24,r0
			xor			r10,r10,r10									//y width index
			xor			r27,r27,r27									//uv width index

			xor			r26,r26,r26									//store index


		w_loop:
			lvx			v10,r6,r10									//16 y's
			addi		r10,r10,16
			
			lvsl		v8,r7,r27									//u alignment vector
			vmrghb		v13,v7,v10									//8 unsigned short y's
	cmpw 		r9,r10

			lvsl		v9,r8,r27									//v alignment vector
			vmrglb		v14,v7,v10									//8 unsigned short y's

			lvx			v11,r7,r27									//only interested in 8 u's
			vsubuhm		v13,v13,v5									//y - 16
			
			lvx			v12,r8,r27									//only interested in 8 v's
			vsubuhm		v14,v14,v5									//y - 16
			addi		r27,r27,8

			vslh		v13,v13,v31									

			vslh		v14,v14,v31		

			vperm		v11,v11,v11,v8
			vsrh		v8,v13,v30									//get sign bit
			
			vperm		v12,v12,v12,v9		
			vsrh		v9,v14,v30									//get sign bit

			vmhaddshs	v13,v13,v0,v8								//(1.164 * 32768)(y - 16)
			vmrghb		v11,v7,v11									//convert to unsigned short

			vmhaddshs	v14,v14,v0,v9								//(1.164 * 32768)(y - 16)
			vmrghb		v12,v7,v12									//convert to unsigned short

			vsubuhm		v11,v11,v6									//(Cb - 128)

			vsubuhm		v12,v12,v6									//(Cr - 128)

			vslh		v11,v11,v31

			vslh		v12,v12,v31
			vmrghh		v15,v11,v11									//double (Cb - 128)'s
			
			vmrghh		v16,v12,v12									//double (Cr - 128)'s
			vsrh		v20,v15,v30									//get sign bits		

			vsrh		v21,v16,v30									//get sign bits		

			vmhaddshs	v9,v15,v4,v20								//CbforB							
			vmrglh		v11,v11,v11									//double (Cb - 128)'s
			
			vaddshs		v9,v9,v13									//b

			vmrglh		v12,v12,v12									//double (Cr - 128)'s
			vmhaddshs	v8,v16,v1,v21								//CrforR
			
			vaddshs		v8,v8,v13									//r

			vaddshs		v8,v8,v29									//add in dither

			vpkshus		v8,v8,v8									//8 r's
			vmhaddshs	v16,v16,v2,v21								//CrforG

			vaddshs		v9,v9,v29									//add in dither			
			
			vpkshus		v9,v9,v9									//8 b's
			vmhaddshs	v15,v15,v3,v20								//CbforG
			
			vsubshs		v13,v13,v16									//almost g

			vsubshs		v13,v13,v15 								//g
								
			vaddshs		v13,v13,v29									//add in dither

			vsrh		v22,v11,v30									//get sign bits	(Cb - 128)	

			vsrh		v23,v12,v30									//get sign bits	(Cr - 128)	

			vpkshus		v13,v13,v13									//8 g's				
			vmhaddshs	v15,v12,v1,v23								//second CrforR's

			vaddshs		v15,v15,v14									//second r's
	 			
			vmrghb		v8,v7,v8									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			vmhaddshs	v16,v11,v4,v23								//second CbforB's
			
			vaddshs		v16,v16,v14									//second b's
			
			vmrghb		v20,v13,v9									//g b g b g b g b g b g b g b g b 
			vmhaddshs	v12,v12,v2,v23								//second CrforG's

			vmrghh		v21,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b
			vmhaddshs	v11,v11,v3,v22								//second CbforG's					

			vsubshs		v14,v14,v12									//almost second g's

			vsubshs		v14,v14,v11									//second g's
			vmrglh		v20,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b

			vaddshs		v15,v15,v29									//add in dither

			vpkshus		v15,v15,v15									//8 r's
			vaddshs		v16,v16,v29									//add in dither
						
			vpkshus		v16,v16,v16									//8 b's
			vaddshs		v14,v14,v29									//add in dither

			vpkshus		v14,v14,v14									//8 g's

			vpkpx 		v20,v21,v20

			stvx 		v20,r3,r26
			vmrghb		v11,v7,v15									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			addi		r26,r26,16
			
			vmrghb		v14,v14,v16									//g b g b g b g b g b g b g b g b 
			
			vmrghh		v12,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b
			
			vmrglh		v11,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b

			vpkpx 		v11,v12,v11
			
			stvx 		v11,r3,r26
			addi		r26,r26,16
			bne      	w_loop                   

		    slwi        r10,r30,31
		    addi        r30,r30,1               					//;inc line ptr

			li			r26,0x3
		    cmpw        r30,r31

		    srawi       r10,r10,31              					//;generate mask
		    subf        r6,r29,r6               					//;next line of y's

		    and         r10,r10,r28             					//;will be 0 or UVStride
			and 		r0,r30,r26									//get dither index

			slwi		r0,r0,4
		    add         r3,r3,r4                					//;next scan line
		    
		    subf        r7,r10,r7               
		    subf        r8,r10,r8
		    bne         h_loop
		} 
	}
	else
	{
		/* destination is NOT 16 byte aligned */
		/* NOTE: compiler uses r25 for vrsave */	


		/**/
		 bcs00(_ptrScreen, thisPitch, src);
		 
/*
		asm
		{
		    lwz         r24,vp3_ditherPats_av
		    xor			r0,r0,r0

		    lwz         r26,vp3_vConst1
		    xor         r30,r30,r30             					//;clear hIndex
			vspltisb 	v30,15
		    
			lvx			v0,r26,r30
			vxor		v7,v7,v7
			vspltisb 	v31,2
			
		    lwz         r6, YUV_BUFFER_CONFIG.YBuffer(r5)
			vsplth		v1,v0,1										//1.596 * 32768
			
		    lwz         r7, YUV_BUFFER_CONFIG.UBuffer(r5)
			vsplth		v2,v0,2										//-0.813 * 32768

		    lwz         r8, YUV_BUFFER_CONFIG.VBuffer(r5)
			vsplth		v3,v0,3										//-0.392 * 32768

		    lwz         r9, YUV_BUFFER_CONFIG.YWidth(r5)
			vsplth		v4,v0,4										//2.017 * 32768

		    lwz         r31,YUV_BUFFER_CONFIG.YHeight(r5)
			vsplth		v5,v0,5										//16

		    lwz         r29,YUV_BUFFER_CONFIG.YStride(r5)
			vsplth		v6,v0,6										//128

		    lwz         r28,YUV_BUFFER_CONFIG.UVStride(r5)
			vsplth		v0,v0,0										//1.164 * 32768

		h_loop_ual:
			lvx			v29,r24,r0
			xor			r10,r10,r10									//y width index
			xor			r27,r27,r27									//uv width index

			xor			r26,r26,r26									//store index


		w_loop_ual:
			lvx			v10,r6,r10									//16 y's
			addi		r10,r10,16
			
			lvsl		v8,r7,r27									//u alignment vector
			vmrghb		v13,v7,v10									//8 unsigned short y's
	cmpw 		r9,r10

			lvsl		v9,r8,r27									//v alignment vector
			vmrglb		v14,v7,v10									//8 unsigned short y's

			lvx			v11,r7,r27									//only interested in 8 u's
			vsubuhm		v13,v13,v5									//y - 16
			
			lvx			v12,r8,r27									//only interested in 8 v's
			vsubuhm		v14,v14,v5									//y - 16
			addi		r27,r27,8

			vslh		v13,v13,v31									

			vslh		v14,v14,v31		

			vperm		v11,v11,v11,v8
			vsrh		v8,v13,v30									//get sign bit
			
			vperm		v12,v12,v12,v9		
			vsrh		v9,v14,v30									//get sign bit

			vmhaddshs	v13,v13,v0,v8								//(1.164 * 32768)(y - 16)
			vmrghb		v11,v7,v11									//convert to unsigned short

			vmhaddshs	v14,v14,v0,v9								//(1.164 * 32768)(y - 16)
			vmrghb		v12,v7,v12									//convert to unsigned short

			vsubuhm		v11,v11,v6									//(Cb - 128)

			vsubuhm		v12,v12,v6									//(Cr - 128)

			vslh		v11,v11,v31

			vslh		v12,v12,v31
			vmrghh		v15,v11,v11									//double (Cb - 128)'s
			
			vmrghh		v16,v12,v12									//double (Cr - 128)'s
			vsrh		v20,v15,v30									//get sign bits		

			vsrh		v21,v16,v30									//get sign bits		

			vmhaddshs	v9,v15,v4,v20								//CbforB							
			vmrglh		v11,v11,v11									//double (Cb - 128)'s
			
			vaddshs		v9,v9,v13									//b

			vmrglh		v12,v12,v12									//double (Cr - 128)'s
			vmhaddshs	v8,v16,v1,v21								//CrforR
			
			vaddshs		v8,v8,v13									//r

			vaddshs		v8,v8,v29									//add in dither

			vpkshus		v8,v8,v8									//8 r's
			vmhaddshs	v16,v16,v2,v21								//CrforG

			vaddshs		v9,v9,v29									//add in dither			
			
			vpkshus		v9,v9,v9									//8 b's
			vmhaddshs	v15,v15,v3,v20								//CbforG
			
			vsubshs		v13,v13,v16									//almost g

			vsubshs		v13,v13,v15 								//g
								
			vaddshs		v13,v13,v29									//add in dither

			vsrh		v22,v11,v30									//get sign bits	(Cb - 128)	

			vsrh		v23,v12,v30									//get sign bits	(Cr - 128)	

			vpkshus		v13,v13,v13									//8 g's				
			vmhaddshs	v15,v12,v1,v23								//second CrforR's

			vaddshs		v15,v15,v14									//second r's
	 			
			vmrghb		v8,v7,v8									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			vmhaddshs	v16,v11,v4,v23								//second CbforB's
			
			vaddshs		v16,v16,v14									//second b's
			
			vmrghb		v20,v13,v9									//g b g b g b g b g b g b g b g b 
			vmhaddshs	v12,v12,v2,v23								//second CrforG's

			vmrghh		v21,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b
			vmhaddshs	v11,v11,v3,v22								//second CbforG's					

			vsubshs		v14,v14,v12									//almost second g's

			vsubshs		v14,v14,v11									//second g's
			vmrglh		v20,v8,v20									//0 r g b 0 r g b 0 r g b 0 r g b

			vaddshs		v15,v15,v29									//add in dither

			vpkshus		v15,v15,v15									//8 r's
			vaddshs		v16,v16,v29									//add in dither
						
			vpkshus		v16,v16,v16									//8 b's
			vaddshs		v14,v14,v29									//add in dither

			lvsr		v10,r3,r26
			vpkshus		v14,v14,v14									//8 g's

			vperm		v20,v20,v20,v10

			vperm		v21,v21,v21,v10
			
			vpkpx 		v20,v21,v20

			stvehx 		v20,r3,r26									//pixel 0
			vmrghb		v11,v7,v15									//0 r 0 r 0 r 0 r 0 r 0 r 0 r 0 r
			addi		r26,r26,2
			
			stvehx 		v20,r3,r26									//pixel 1
			addi		r26,r26,2
			vmrghb		v14,v14,v16									//g b g b g b g b g b g b g b g b 
			
			stvehx 		v20,r3,r26									//pixel 2
			addi		r26,r26,2
			vmrghh		v12,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b
			
			stvehx 		v20,r3,r26									//pixel 3
			addi		r26,r26,2
			vmrglh		v11,v11,v14									//0 r g b 0 r g b 0 r g b 0 r g b

			stvehx 		v20,r3,r26									//pixel 4
			addi		r26,r26,2

			stvehx 		v20,r3,r26									//pixel 5
			addi		r26,r26,2

			stvehx 		v20,r3,r26									//pixel 6
			addi		r26,r26,2

			stvehx 		v20,r3,r26									//pixel 7
			addi		r26,r26,2

			lvsr		v10,r3,r26

			vperm		v11,v11,v11,v10

			vperm		v12,v12,v12,v10


			vpkpx 		v11,v12,v11

			stvehx 		v11,r3,r26									//pixel 8
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 9
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 10
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 11
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 12
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 13
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 14
			addi		r26,r26,2

			stvehx 		v11,r3,r26									//pixel 15
			addi		r26,r26,2
			bne      	w_loop_ual                   

		    slwi        r10,r30,31
		    addi        r30,r30,1               					//;inc line ptr

			li			r26,0x3
		    cmpw        r30,r31

		    srawi       r10,r10,31              					//;generate mask
		    subf        r6,r29,r6               					//;next line of y's

		    and         r10,r10,r28             					//;will be 0 or UVStride
			and 		r0,r30,r26									//get dither index

			slwi		r0,r0,4
		    add         r3,r3,r4                					//;next scan line
		    
		    subf        r7,r10,r7               
		    subf        r8,r10,r8
		    bne         h_loop_ual
		} 
*/
	}
}

