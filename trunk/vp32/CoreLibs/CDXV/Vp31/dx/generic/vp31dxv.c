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


/*
 dxvmpg.cpp : Defines the entry point for the console application.
*/
#include <stdlib.h>

#include "dkpltfrm.h" /* platform specifics */

#include "duck_mem.h" /* interface to memory manager */
#include "dxl_main.h" /* interface to dxv */

#include "vp3d.h"
#include "vfw_pb_interface.h"
 
#define VP31_FOURCC DXL_MKFOURCC( 'V', 'P', '3', '1')

extern void vp3SetBlit(void);
extern void VPInitLibrary(void);
extern void VPDeInitLibrary(void);

/* define an xImage structure based on the core xImage struct */
typedef struct tXImageCODEC
{
	xImageBaseStruct;
	FourCC myFourCC;
	YUVbufferLayout *myNewFrameBufferPtr;
	YUV_BUFFER_CONFIG FrameBuffer;
	xPB_INST myPBI;
	
} VP31_XIMAGE,*VP31_XIMAGE_HANDLE;

static dxvBitDepth bitDepths[] = 
{
	DXRGB32,DXRGB24,DXRGB16,DXRGBNULL
};


typedef void ((*VP3BLIT_FUNC)(unsigned char *, int, YUV_BUFFER_CONFIG *));
typedef void ((*VP3_VSCREEN_FUNC)(void));


DXL_INTERNAL_FORMAT vp31_GetXImageInternalFormat(DXL_XIMAGE_HANDLE xImage,
												DXL_VSCREEN_HANDLE vScreen)
{
	return YV12;
}


static int vp31_decompress(VP31_XIMAGE_HANDLE src, DXL_VSCREEN_HANDLE vScreen)
{

	unsigned int PostProcess;
	if(!vScreen)
	{
		GetPbParam( src->myPBI, 0, &PostProcess);
		SetPbParam( src->myPBI,0,1);
	};

    if (src->addr)
    {

        int retVal= DecodeFrameToYUV(src->myPBI,
            (char *)src->addr, src->fSize, src->imWidth, src->imHeight);
        if(retVal != 0 )
        {
            if(retVal == -1)
                return DXL_VERSION_CONFLICT;
            else
                return DXL_BAD_DATA;
        }
    }
	if(!vScreen)
	{
		SetPbParam( src->myPBI, 0, PostProcess);
	};

//else
// probably should do a return

    GetYUVConfig(src->myPBI, &(src->FrameBuffer));

    /* this is needed by DXL_GetXImageXYWH */
//	src->imWidth = src->w = src->FrameBuffer.YWidth;
//	src->imHeight = src->h = src->FrameBuffer.YHeight;

	if (vScreen) /* if there is a vScreen, blit to it */
	{
		if (vScreen->addr)
        { 
    		int x,y,pSize;
            int w,h;
            unsigned char *ptrScrn;
            int thisPitch = vScreen->pitch;

            pSize = DXL_GetVScreenSizeOfPixel(vScreen);

		    /* remember to offset if requested */
		    y = vScreen->viewY + src->y;           
		    x = vScreen->viewX + src->x;

            /* for planar destinations */
            w = vScreen->pitch;
            h = vScreen->height;

		    ptrScrn = vScreen->addr;
	        ptrScrn += (x * pSize) + (y * thisPitch);

            /* setup ptrs so we can work backwards through the frame buffers */
            src->FrameBuffer.YBuffer = src->FrameBuffer.YBuffer + 
                    ((src->FrameBuffer.YHeight - 1) * 
                     (src->FrameBuffer.YStride));

			src->FrameBuffer.UBuffer = src->FrameBuffer.UBuffer +
                    ((src->FrameBuffer.UVHeight - 1) * 
                     (src->FrameBuffer.UVStride));
			
            src->FrameBuffer.VBuffer = src->FrameBuffer.VBuffer +
                    ((src->FrameBuffer.UVHeight - 1) * 
                     (src->FrameBuffer.UVStride));


//            if((vScreen->bd != DXYUY2) || (vScreen->bd != DXYV12))
            if((vScreen->bd != DXYUY2) && (vScreen->bd != DXYV12))
            {
                if(vScreen->bq == DXBLIT_STRETCH)
                {
                    thisPitch *= 2;
                }
            }


            if (vScreen->blitSetup != (void *)-1) 
                ((VP3_VSCREEN_FUNC)vScreen->blitSetup)();

            if ((VP3BLIT_FUNC)vScreen->blitter == (VP3BLIT_FUNC)-1)
                return DXL_INVALID_BLIT;

            ((VP3BLIT_FUNC)vScreen->blitter)(ptrScrn, thisPitch, (&src->FrameBuffer));

            if ((VP3BLIT_FUNC)vScreen->blitExit != (VP3BLIT_FUNC)-1) 
                ((VP3_VSCREEN_FUNC)vScreen->blitExit)();

        }
	}

	return DXL_OK;
}

/* 
  close down a decompressor, releasing the decompressor, 
  the xImage (decompressor), and the intermediate vScreen (surface)
*/

static int vp31_xImageDestroy(VP31_XIMAGE_HANDLE xThis)
{
	if (xThis)
	{
        StopDecoder(&(xThis->myPBI));
		duck_free(xThis);
	}

	return DXL_OK;
}

/* 
  called during initialization and/or when xImage (decompressor)
  attributes change, note that nImage and src are actually
  synonymous and should be cleared out a bit (to say the least!)


  !!!!!!
  This function should be prepared to get data that is NOT of the 
  type native to the decoder,  It should do it's best to verify it 
  as valid data and should clean up after itself and return NULL
  if it doesn't recognize the format of the data
*/
static DXL_XIMAGE_HANDLE vp31_xImageReCreate(VP31_XIMAGE_HANDLE src,unsigned char *data,
	int type,enum BITDEPTH bitDepth,int w,int h)
{  
    DXL_XIMAGE_HANDLE vp31_xImageCreate(unsigned char *data);
	
    if (type != VP31_FOURCC) 
		return NULL;

	if (src != NULL)	/* if an xImage/decompressor already exists, destroy it */
		vp31_xImageDestroy(src);

	/* create a new xImage, specific to this type of decoder, 
        (see "VP31_XIMAGE" struct above and dxl_main.h) */

	src = (VP31_XIMAGE_HANDLE)duck_calloc(1,sizeof(VP31_XIMAGE),DMEM_GENERAL);

	if (!src) 
        return NULL;

//	duck_memset(nImage,0,sizeof(VP31_XIMAGE));

	/* set up the "vtable" of interface calls */
    src->create =  (DXL_XIMAGE_HANDLE (*)(void *)) vp31_xImageCreate;
    src->recreate =  (DXL_XIMAGE_HANDLE (*)(DXL_XIMAGE_HANDLE,void *,int,int,int,int)) vp31_xImageReCreate;

	src->destroy = (int (*)(DXL_XIMAGE_HANDLE))vp31_xImageDestroy;
	src->dx = (int (*)(DXL_XIMAGE_HANDLE, DXL_VSCREEN_HANDLE)) vp31_decompress;
	src->blit = NULL; /* there is no interleaved blitter for vp3x files */

    src->internalFormat = (int (*)(DXL_XIMAGE_HANDLE, DXL_VSCREEN_HANDLE)) vp31_GetXImageInternalFormat; 
	src->bdPrefs = bitDepths; /* plug in the list of prefered bit depths */

    src->addr = data;
    src->dkFlags.inUse = 1;

	src->imWidth = src->w = w ? w : 320;
	src->imHeight = src->h = h ? h : 240;

	src->myFourCC = VP31_FOURCC;
  
    /* create new PBI */
    if(!StartDecoder( &(src->myPBI), src->imWidth, src->imHeight ))
    {
		vp31_xImageDestroy(src);
        src = NULL;
    }

    return (DXL_XIMAGE_HANDLE ) src;
}

/* in this "glue" case, just calls through to the create function */

static DXL_XIMAGE_HANDLE vp31_xImageCreate(unsigned char *data)
{
	return vp31_xImageReCreate(NULL, data, VP31_FOURCC, (enum BITDEPTH ) 0,0,0);
}

int vp31_Init(void)
{

    DXL_RegisterXImage( 
		(DXL_XIMAGE_HANDLE (*)(unsigned char *)) vp31_xImageCreate,
		VP31_FOURCC, 
        YV12 
		);
	
    vp3SetBlit();

	/* initialize all the global variables */
	VPInitLibrary();

	return DXL_OK;
}

/* 
    main exit routine, called during DXL_ExitVideo() 
    clean up any global information if necessary
*/

int vp31_Exit(void)
{
	VPDeInitLibrary();

	return DXL_OK;
}

void vp31_SetParameter(DXL_XIMAGE_HANDLE src,int Command, unsigned long Parameter )
{
	SetPbParam( ((VP31_XIMAGE_HANDLE) src)->myPBI, (PB_COMMAND_TYPE) Command, (UINT32) Parameter );
}