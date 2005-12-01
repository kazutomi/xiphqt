/*
 * 05.03.2002 changing for revA
 * 03.19.2002 Started support for different sensors
 */

#ifndef _ASM_CMOSCAM_H
#define _ASM_CMOSCAM_H

/* _IOC_TYPE, bits 8 to 15 in ioctl cmd */
#define CMOSCAM_IOCTYPE 124

/* MINORS */

#define CMOSCAM_MINOR_FRAME  1
#define CMOSCAM_MINOR_FPN  2
//#define CMOSCAM_MINOR_IORW  1
//#define CMOSCAM_MINOR_I2C  2
//#define CMOSCAM_MINOR_RAW  3
#define CMOSCAM_MINOR_I2C  3
#define CMOSCAM_MINOR_DMA  4
#define CMOSCAM_MINOR_MCP  5
#define CMOSCAM_MINOR_LOCK  6
#define CMOSCAM_MINOR_UNLOCK  7
#define CMOSCAM_MINOR_SENSORJTAGFPGA 8
#define CMOSCAM_MINOR_RWTABLES 9
#define CMOSCAM_MINOR_SENSORFPGACOMM 10
#define CMOSCAM_MINOR_CIRCBUF 11
#define CMOSCAM_MINOR_CIRCINDEX 12

/* camera sequencer states */

//New sequencer states

#define CAMSEQ_OFF       0 // off, not programmed (Video mode off on Zoran sensors)
#define CAMSEQ_READY     1 // sensor programmed may acquire at will (programSensor sets number of frames to skip (if any)
#define CAMSEQ_SKIP      2 // skipping specified number of frames, interrupt service routine counts and will start acquisition
#define CAMSEQ_WAIT_F    3 // set by "start exposure" or interrupt service routine. WAIT_F/WAIT_T/acquire/done differs by hardware register
#define CAMSEQ_WAIT_T    4 // set by "start exposure" or interrupt service routine. Wait/acquire/done differs by hardware register
#define CAMSEQ_ACQUIRE   5 // acquisition in progress (camSeqState is still CAMSEQ_WAIT)
#define CAMSEQ_DONE      6 // acquisition over  (camSeqState is still CAMSEQ_WAIT)
#define CAMSEQ_JPEG      7 // waiting for JPEG done interrupt, acquiring/compressing some frames

#define CAMSEQ_RUN       8 // compressor is constantly running (but if camSeqCount>0 - just skipping "bad" frames)
#define CAMSEQ_STOP      9 // compressor is constantly running but will stop after next "compressor ready"
#define CAMSEQ_SINGLE   10 // compressor is constantly running to all the buffer
/*
Status CAMSEQ_OFF may be changed to CAMSEQ_READY only by programSensor, it will also stop sensor (if needed)
  and set camSeqCount (if needed) to skip certain number of frames before actual acquisition
  camSeqStart (called mostly through ioctl) will return error if state was not "CAMSEQ_READY" or "CAMSEQ_DONE".
  If called from correct states will either start ISR-based waiting for specified number of frames  or just acquisition.
  In the first case ISR will start acquisition itself
  State "CAMSEQ_WAIT_F" is set at the same time as starting acquisition (by either camSeqStart() or ISR),
  States CAMSEQ_WAIT_F,CAMSEQ_ACQUIRE, CAMSEQ_DONE are returned after testing the hardware register in FPGA if camSeqState==CAMSEQ_WAIT_F
  camSeqStop will terminate any acquisition in progress and set state to CAMSEQ_READY (if it was not CAMSEQ_OFF)
*/
// For KAI11000 sensor board
#define  sensorcom_W_size     1024
#define  sensorcom_R_size      256

/* MCP definitions */

#define  MCP_W_size        1024
#define  MCP_R_size         256

#define  MCPOtherBits  0xffa7a7ff
#define MCPOffReset  0x00101800
#define MCPReset  0x00001800
#define MCPNoReset  0x00105800
#define MCPToggleA  0x00080000
#define MCPToggleB  0x00001000
#define MCPctlseq  0x00
#define MCPsofttg  0x02
#define MCPeackn  0x03
#define MCPctlgate  0x04
#define MCPwstdly  0x06
#define MCPwrsmsk  0x07
#define MCPwrsup  0x08
#define MCPwrmons  0x09
#define MCPwnom    0x0a
#define MCPwdenom  0x0b
#define MCPwoutw  0x0c
#define MCPwinvctl  0x0d
#define MCPctlsync  0x0e
#define MCPwrdlys  0x10
#define MCPwinv    0x40
#define MCPwshared  0x80
#define MCPwrsynctb  0x100
#define MCPwrseq  0x200


/* supported ioctl _IOC_NR's */
#ifndef I2C_WRITEARG
#define I2C_WRITEARG(bus, slave, reg, value) (((bus) << 24) | ((slave) << 16) | ((reg) << 8) | (value))
#define I2C_READARG(bus, slave, reg) (((bus) << 24) | ((slave) << 16) | ((reg) << 8))

#define I2C_ARGBUS(arg) (((arg) >> 24)  & 0x1)
#define I2C_ARGSLAVE(arg) (((arg) >> 16)  & 0xff)
#define I2C_ARGREG(arg) (((arg) >> 8) & 0xff)
#define I2C_ARGVALUE(arg) ((arg) & 0xff)

#define I2C_WRITEREG 0x1   /* write to an i2c register */
#define I2C_READREG  0x2   /* read from an i2c register */
#endif

/* new for Micron sensors  - 16bit data, always bus 0 */
#ifndef I2C_16_WRITEARG
#define I2C_16_WRITEREG 0x3   /* write 2 bytes to an i2c register */
#define I2C_16_READREG  0x4   /* read 2 bytes from an i2c register */
  
#define I2C_16_WRITEARG(slave, reg, value) (((slave) << 24) | ((reg) << 16) | (value))
#define I2C_16_READARG(slave, reg) (((slave) << 24) | ((reg) << 16))

#define I2C_16_ARGSLAVE(arg)   (((arg) >> 24) & 0xff)
#define I2C_16_ARGREG(arg)     (((arg) >> 16) & 0xff)
#define I2C_16_ARGVALUE(arg)   ( (arg)      & 0xffff)
#define I2C_16_ARGVALUE_H(arg) (((arg) >>  8) & 0xff)
#define I2C_16_ARGVALUE_L(arg) ( (arg)        & 0xff)
#endif



// otherParamsRO[16]

#define _CCCMD(x,y)  (_IO(CMOSCAM_IOCTYPE, (x << 6) | (y & 0x3f)))



#define CCAM_CTRL(x)  ((_IOC_NR(x) >> 6) & 0x03)
#define CCAM_ADDR(x)  (_IOC_NR(x) & 0x3f)

//#define CCAM_RWSENSOR   1 /* direct read/write first 32 sensor registers */  // will not use at all
#define CCAM_RPARS      2 /* read  parameters      0..0x3f */
#define CCAM_WPARS      3 /* write parameters      0..0x3f */

/* New parameters and update logic
 * Separate read and write set of 64 registers
 * User may specify:
 *   0 - do not update
 *   1 - update at once
 *   2 - update when appropriate
 * and read update status:
 *   0 - will not be updated
 *   1 - in sync
 *   2 - waiting to be updated
 *   3 - update in progress (TODO: - support async)
 *   When updating (validating) parameters and copying them to the read "registers" the I2C registers will be written
 *  only if they are different from the shadows
 */

/* abstract sensor register names */
#define P_UPDATE         0      /* R/W, see above */
#define P_SENSOR         1     /* read only
                           4  - ZR32112MLC - now there is no way to see color/mono
                           5  - ZR32112PLC
                           8  - ZR32212MLC
                           9  - ZR32112PLC
                           32 - KAC1310-mono
                           33 - KAC1310-RGB
                           34 - KAC1310-CMY
                           48 - MI1300
                           49 - MT9M001 (1280x1024,same as MI1300)
                           50 - MT9M001 (1600x1200)
                           51 - MT9T001 (2048*1536)
                           64 - IBIS5-1300*/

// leave it here - may be used in user applications
#define SENSOR_MASK      0xfc
#define SENSOR_ZR32112   0x04
#define SENSOR_ZR32212   0x08
#define SENSOR_KAC1310   0x20
#define SENSOR_MI1300    0x30
#define SENSOR_MT9X001   0x30 // MT9M001 - 1, MT9D001 - 2, MT9T001 - 3
#define SENSOR_IBIS51300 0x40
#define SENSOR_KAI11000  0x80
#define SENSOR_NONE      0xfc

// sensor sizes:
//#define SENSORWIDTH_ZR32112  1288
//#define SENSORHEIGHT_ZR32112 1032
#define SENSORWIDTH_ZR32112  1280
#define SENSORHEIGHT_ZR32112 1024
//#define SENSORWIDTH_ZR32212  1288
#define SENSORWIDTH_ZR32212  1280
#define SENSORHEIGHT_ZR32212 968
#define SENSORWIDTH_KAC1310  1280
#define SENSORHEIGHT_KAC1310 1024
//#define SENSORWIDTH_KAC1310  1296
//#define SENSORHEIGHT_KAC1310 1046
#define SENSORWIDTH_MI1300  1280
#define SENSORHEIGHT_MI1300 1024

#define SENSORWIDTH_MT9M001  1280
#define SENSORHEIGHT_MT9M001 1024
#define SENSORWIDTH_MT9D001  1600
#define SENSORHEIGHT_MT9D001 1200
#define SENSORWIDTH_MT9T001  2048
#define SENSORHEIGHT_MT9T001 1536

#define SENSORWIDTH_IBIS51300  1280
#define SENSORHEIGHT_IBIS51300 1024

#define P_PARS_CHANGED   2 /* RD P_RO_VALID  0  window parameters valid                  */
#define P_DMA_VALID      3 /* RD P_RO_VALID  0  window parameters valid                  */
#define P_ACTUAL_WIDTH   4 /* RD P_RO_WIDTH  1  pixels/row                               */
#define P_ACTUAL_HEIGHT  5 /* RD P_RO_HEIGHT 2  pixels/column                            */
#define P_LPR            6 /* RD P_RO_LPR    3  32-bit words per row (aligned each row)  */
#define P_IMAGE_SIZE     7 /* RD P_RO_SIZE   4  total image size (dma data) in bytes     */
#define P_BAYER          8 /* filter number at (0,0) 0-R, 1-G(R), 2-G(B), 3 - B. Write enabled at first, move to WindowSize later */
#define P_TRIGGERED     9 /* when trigger occured - 4 LSBs - pixel in DMA word, higher bits - number of DMA word */
#define P_PERIOD        10 // Frame period (read only)
#define P_FRAME         11 // Frame number (reset with JPEG pointers) -(read only)
#define P_JPEG_WP       12 // Last reported JPEG write pointer in the circular buffer. May need to be adjustment
#define P_CLK_FPGA      13 // FPGA clock in MHz
#define P_CLK_SENSOR    14 // Sensor clock in MHz
#define P_FPGA_XTRA     15 // Extra cycles needed to compressor (probably constant...)

//#define P_TRIG          16 /* R/W P_RW_TRIG   0   0 - internal, 1 - external               */
#define P_TRIG          16 /* External trigger mode                                           */
                           /* bit 0  - "old" external mode (0- internal, 1 - external )       */
                           /* bit 1 - enable(1) or disable(0) external trigger to stop clip   */
                           /* ... to be continued */
#define P_EXPOS         17 /* P_RW_EXPOS  1   exposure time                            */
#define P_BGFRAME       18 // Background measurement mode - will use 16-bit mode and no FPN correction 

//#define P_DMA_SZ        18 // P_RW_DMA_SZ 2   DMA block size (debug, mostly)
//#define P_DMA_LOOP      19 // Cyclic DMA buffer (should be updated to multiple of frames)





// image page numbers depend on image size/pixel depth, so changing any of them will invalidate all pages
#define P_PAGE_ACQ      19  // Number of image page buffer to acquire to (0.1?)
#define P_PAGE_READ     20  // Number of image page buffer to read from to (0.1?)

#define P_OVERLAP       21 // number of EXRA lines to be acquired
#define P_VIDEO         22 /* still (stopped) - 0, video - 1. KAC1310 - video only */

#define P_AUXCM    23 /* signal on AUXCLK pin: 0 - 0, 1 - 1, 1x - 20MHz */

#define P_VIRT_WIDTH    24 /* Virtual window width - KAC-1310 sensor */
#define P_VIRT_HEIGHT   25 /* Virtual window height - KAC-1310 sensor */
#define P_WOI_LEFT      26 /* WOI left corner (before applying decimation) */
#define P_WOI_TOP       27 /* WOI top corner */
#define P_WOI_WIDTH     28 /* WOI width */
#define P_WOI_HEIGHT    29 /* WOI height */

#define P_FLIPH         30
#define P_FLIPV         31
#define P_DCM_HOR       32 /* Horizontal decimation (1/2/4/8) */
#define P_DCM_VERT      33 /* Vertical   decimation (1/2/4/8) copied from horizontal for Zoran chips */
#define P_BIN_HOR       34 /* binning 1/2 - KAC1310 only */
#define P_BIN_VERT      35 /* not used yet binning 1/2 - KAC1310 only */

#define P_COLOR         36 /* mono - 0, color mode - 1 */
#define P_TEST          37 // 0 - normal, 1 - sensor test, 2 - FPGA test
#define P_MCLK          38 /* 2..129 */
#define P_MCLK_DIS      39 /* disable sensor clock */
             
#define P_BITS          40 /* pixel depth - bits 10/8/4 */
#define P_SHIFTL        41 /* "digital gain" - shift left by 0/1/2 bits  (3 ->-1)*/
#define P_FPNS          42 // FPN correction mode (subtract) 0..3
                           // 0-none, 1 - fine(25%), 2 - 50%, 3 - coarse(100%)
#define P_FPNM          43 // FPN correction mode (multiply) 0..3
                           // 0-none, 1 - fine(+/-12.5%), 2 - medium (+/-25%), +3 - coarse(+/-50%)
#define P_VEXPOS        44 /* video exposure (if 0 - use P_RW_EXPOS in ms) */
#define P_VIRTTRIG      45 // Sum of pixels in a line greater than this value - trigger acquisition

#define P_GAINR         46 /* R channel gain (mono gain) */
#define P_GAING         47 /* G channel gain ("red line" in ZR32212) */
#define P_GAINB         48 /* B channel gain (mono gain) */
#define P_GAINGB        49 /* G channel gain ("blue line" in ZR32212) - copied from P_RW_GAING in ZR2112 */
#define P_FATZERO       50 // subtract while adding data from to consequitive frames (async trigger)
// will be more for KODAK sensor ?
#define P_SPEC_KAC_GGA  51 // KAC-1310 Global gain A , 6 bits, (14)
#define P_SPEC_KAC_GGB  52 // KAC-1310 Global gain B , 6 bits, (14)
#define P_SPEC_KAC_GGM  53 // KAC-1310 Global gain mode, 3 bits, 0
#define P_QUALITY       54 // JPEG IMAGE QUALITY
#define P_FP100S        55 // Frames per 100 sec (fps * 100)
#define P_SENSOR_WIDTH  56
#define P_SENSOR_HEIGHT 57
#define P_COLOR_SATURATION_BLUE 58 // 100*realtive saturation blue
#define P_COLOR_SATURATION_RED  59 // 100*realtive saturation red
#define P_GAMMA         60 // 100*gamma  (for now - just a hint table could be more complex. Use higher bytes for colors in 333)
#define P_PIXEL_LOW     61 //  (just a hint for gamma table) "fat zero" (on 8-bit scale)
#define P_PIXEL_HIGH    62 //  (just a hint for gamma table) highest pixel value (before table) on a 256-scale


#define P_DONTCARE      63 // write anything - just to force update




/*
EXAMPLE usage:

    i2c_arg = I2C_WRITEARG(STA013_WRITE_ADDR, reg, val);
    ioctl(fd, _IO(ETRAXI2C_IOCTYPE, I2C_WRITEREG), i2c_arg);

    i2c_arg = I2C_READARG(STA013_READ_ADDR, reg);
    val = ioctl(fd, _IO(ETRAXI2C_IOCTYPE, I2C_READREG), i2c_arg);


*/



/* i2c errors */
#ifndef ERR_I2C_SCL_ST0
#define  ERR_I2C_SCL_ST0     1
#define  ERR_I2C_SDA_ST0     2
#define  ERR_I2C_SCL_ST1     4
#define  ERR_I2C_SDA_ST1     8
#define  ERR_I2C_SCL_NOPULLUP 16
#define  ERR_I2C_SDA_NOPULLUP 32

/* i2c_diagnose called by i2c_start (?) could not find any problems. Try again start */
#define    ERR_I2C_NOTDETECTED  64
#define  ERR_I2C_SHORT     128
#define  ERR_I2C_BSY         256
#define  ERR_I2C_NACK     512
#endif


/* supported ioctl _IOC_NR's */
#define IO_CCAM_SET_EXT_EXPOSURE  0x06
#define IO_CCAM_MONITOR_SEQ       0x07


//#define IO_CCAM_STOP_DMA  0x08
//#define IO_CCAM_START_DMA  0x09 // just starts DMA - descriptor list should be set eatlier
//#define IO_CCAM_START_RAW  0x0a // Programs DMA descriptor list according to current frame size, FPGA registers and starts DMA


#define IO_CCAM_JPEG    0x08 // JPEG-compressor related commands
#define      JPEG_CMD_RESET  0x00 //  Resets pointers - both acquisition and readout
//#define      JPEG_CMD_ARM  0x01 // Prepare compressor to read next frame acquired
#define      JPEG_CMD_GET  0x02 // Read current page (will return empty (and length==0) if not ready
#define      JPEG_CMD_FORGET   0x03 // increment read frame pointer
#define      JPEG_CMD_CATCHUP   0x04 // set read pointer to the last acquired (or acquiring if none is acquired yet)
#define      JPEG_CMD_ACQUIRE   0x05 // acquire and compress one frame
#define      JPEG_CMD_SAVE_RP   0x06 // save read pointer
#define      JPEG_CMD_RESTORE_RP 0x07 // restore read pointer
#define      JPEG_CMD_N_DONE     0x08 // return 1 if no more frames to be acquired (frame number)
#define      JPEG_CMD_L_DONE     0x09 // return 1 if no more frames to be acquired (total length)
#define      JPEG_CMD_START      0x0a // start constant compression mode
#define      JPEG_CMD_STOP       0x0b // stop constant compression mode (may want to wait for CAMSEQ_DONE)
#define      JPEG_CMD_FRAMES     0x0c // returns number of frames in buffer, (re)uilds frames chain
#define      JPEG_CMD_JUST_STOP  0x0d // just stop - don't start cycle if was allready off!
#define      JPEG_CMD_DUMP       0x0f // printk all static data/tables

#define      JPEG_CTRL_MONOCHROME    0x400
#define      JPEG_CTRL_MONOCHROME_BLOCKED    0x1000

#define IO_CCAM_JPEG_QUALITY  0x09 // Set P_QUALITY

#define IO_CCAM_JPEG_GET_N  0x0a  // get specified number of frames (will add to already asked for if any)
#define IO_CCAM_JPEG_GET_L  0x0b  // get specified length (will stop after frame if got more)

#define IO_CCAM_JPEG_CTRL  0x0c // Write JPEG control word (0x10000 - use header, LSW - other settings)
#define IO_CCAM_DMA    0x0d
 #define CCAM_DMA_CMD_STOP    0x00
 #define CCAM_DMA_CMD_START  0x01 // just starts DMA - descriptor list should be set eatlier

#define IO_CCAM_CR_MODIFY 0x0e  //(bit number)<<2 | op; op= 0 - nop, 1 - set, 2 - reset, 3 - toggle)
#define IO_CCAM_CR_SHADOW 0x0f


/* Memory mappable memory... can be mmap'd by application.
*       Contains the following layout:
*       offset                          content
*       -------------------             ---------------------
*       CCAM_MMAP_OFFSET_MMAP_HEADER    struct ccamMmapHeader + fill
*       CCAM_MMAP_OFFSET_JPEG_HEADER    jpeg header to use + fill
*       CCAM_MMAP_OFFSET_DMABUF         dma buffer, contiguous
*/

/* Jpeg dma buffer. Buffer must be a multiple of
*       64Kbyte because one dma descriptor conveniently handles that many...
*       (can not handle more than that also).
*  The #define CCAM_CHUNK_PER_DMABUF can be increased to allow acquiring
*  larger images, if system memory can be freed up elsewhere.
*  A value of 128 means 128*64Kbytes per buffer, or 8Mbyte for buffer,
*  of 16Mbyte total system memory.
*  Jpeg images rarely get as big as 4MB, but they can get bigger.
*/
#define CCAM_BYTES_PER_CHUNK  (1<<16)  /* dma buffer bytes per descriptor */
#define CCAM_DESCR_PER_CHUNK  1
/* If CCAM_CHUNK_PER_DMABUF is 100 then buffer is about 6.5 million bytes
*       which is probably bigger than any single image we'll generate,
*       and is also bigger than raw data (which is 5.5MB).
*       However, images bigger than 6.5/2 == 3.25 million bytes will
*       not be able to be double-buffered and thus will slow things down.
*/
//#define CCAM_CHUNK_PER_DMABUF 102  /* no. of 64Kbyte chunks per buffer */
#define CCAM_CHUNK_PER_DMABUF 300  /* no. of 64Kbyte chunks per buffer */
#define CCAM_WORDS_PER_DMABUF (CCAM_CHUNK_PER_DMABUF<<14) /*32bit words...*/
#define CCAM_BYTES_PER_DMABUF (CCAM_CHUNK_PER_DMABUF<<16)
/*  For past compatibility, CCMA_DMA_SIZE...
*/
#define CCAM_DMA_SIZE CCAM_WORDS_PER_DMABUF
// rather arbitrary - size of indax of frames in buffer, 
// will be limiting factor if frames are less than 256 bytes
#define CCAM_DMA_INDEX_SIZE (CCAM_DMA_SIZE>>6)
/*
*       CCAM_MMAP_OFFSET... -- offsets in bytes in memory mapped region.
*       CCAM_MMAP_SIZE -- no. of bytes to mmap.
*/       
#define CCAM_MMAP_OFFSET_MMAP_HEADER 0
#define CCAM_MMAP_OFFSET_JPEG_HEADER (PAGE_SIZE)
#define CCAM_MMAP_OFFSET_DMABUF (4*PAGE_SIZE)
#define CCAM_MMAP_SIZE (PAGE_SIZE*sizeof(long)+CCAM_BYTES_PER_DMABUF)

// ccamMmapHeader -- Data structure at beginning of mmap'able memory:
// this allows quick access by an application to this data.
// NOTE: this must not exceed PAGESIZE bytes since the jpeg header lives there.
struct ccamMmapHeader
{
    /* Application must not change use_header except through setting
    *  of parameters.
    */
    int use_header;     /* should jpeg header/tailer be added? */
    /* Application must not change headerSize; doing so would only
    *   confuse
    */
    int headerSize;     /* no. of bytes in jpeg header */
    /* Application may set exposureIdx and jpegIdx to any desired value
    *   when no acquisition is going on (e.g. before JPEG_CMD_ACQUIRE).
    */
    int exposureIdx;    /* incremented on every frame sync (exposure) irupt*/
    int jpegIdx;        /* incremented on every jpeg done interrupt */
    /* Data buffering info.
    *   This is readonly by application, which should use:
    *   JPEG_CMD_RESET to zero readOffset and writeOffset and buf[readOffset].
    *   JPEG_CMD_ACQUIRE to acquire image (when done, sets buf[writeOffset]
    *   and then advances writeOffset and zeroes buf[writeOffset]).
    *   JPEG_CMD_FORGET to advance readOffset.
    *
    *   readOffset and writeOffset are byte offsets from beginning of
    *   the dma buffer where one can find the respective ccamFileDataHeader.
    */
    int readOffset;     /* where we (maybe) can read next image from (bytes) */
    int writeOffset;    /* where next image will be placed at (bytes) */
    /* nfr is the number of frames left to acquire in a sequnce of frames 
    */
    int nfr;
    /* lfr is the length (in bytes) of frames left to acquire.
    *   This is a crude measurement that basically says how much of the
    *   dma buffer we're willing to consume.
    */
    int lfr;
    /* While dma of jpeg data is enabled/ongoing, jpegBusy is set nonzero.
    */
    int jpegBusy;
};

// ccamFileDataHeader -- Data structure preceding each image in dma buffer.
// (Note that the actual image may wrap around, but the 
// this struct is always placed so as to avoid wraparound and be aligned).
// Note that this does not include jpeg header or tailer, and any way
// the actual data is in parts (the offset to is given in part[i].offset,
// a byte count relative to start of dma buffer).
struct ccamFileDataHeader
{
    /* Image is not acquired / valid yet until Valid set to 1 or -1  .
    *   A value of -1 indicates buffer overflow error.
    */
    int valid;
    /* Value for "valid: (note, OVERFLOW can be recovered from) */
    #define CCAM_IMAGE_VALIDFLAG_NOTDONE 0  /* MUST be zero */
    #define CCAM_IMAGE_VALIDFLAG_DONEOK  1 
    #define CCAM_IMAGE_VALIDFLAG_OVERFLOW -1  /* buffer was too small */
    #define CCAM_IMAGE_VALIDFLAG_FATAL  -2  /* misc. unexpected error */
    /* nBytes -- Total size of image stored in dma buffer.
    *   This does not include e.g. jpeg header (or this header).
    */
    int nBytes;
    /* nBytesMax -- maximum available during acquisition.
    *   However, the last CCAM_IMAGE_OVERFLOW_BYTES bytes, if used,
    *   should be taken to indicate that an overlow occurred.
    *   The value for CCAM_IMAGE_OVERFLOW_BYTES should be large enough
    *   to accomodate various corrections without adding a bunch of extra logic.
    */
    #define CCAM_IMAGE_OVERFLOW_BYTES 512
    int nBytesMax;
    /* Offset (bytes) in dma buffer of next image header (IFF Valid is 1) */
    int nextOffset;
    /* Offset and size (bytes) in dma buffer of parts of data 
    *   However, only some part(s) may be used, skip those with zero count.
    *   Before acq is done, part[i].nBytes is maximum size of the part;
    *   after acq (when valid is set to 1) it is set to actual amount.
    *   Two parts is enough for a simple ring buffer approach.
    */
    #define CCAM_IMAGE_NPARTS_MAX 2
    struct
    {
        int offset;     /* w/in dma buffer, bytes */
        int nBytes;     /* size of this part */
    } part[CCAM_IMAGE_NPARTS_MAX];
};

#endif /* _ASM_CMOSCAM_H */
