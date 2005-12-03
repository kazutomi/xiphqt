#ifndef __STREAMER_H_DEF_
#define  __STREAMER_H_DEF_

#include <stdint.h>

#define HAVE_NANOSLEEP 1

#define MAX_SIZE_JPEG    1024*1024   // max size buffer for JPEG picture, may be increase for best quality

#define JPEG_HEADER_SIZE 	623
#define RTP_PT_JPEG             26    /* The definition is from RFC1890 */
#define RTP_JPEG_RESTART        0x40
#define BUF_SIZE		1460  // MTU - 40 bytes	
#define TimeScale		90000 // RFC

#define TimestampTicks  	1000000

typedef uint64_t Timestamp;
typedef uint32_t RtpTimestamp;



#ifndef true
#define true		1
#endif
#ifndef false
#define false		0
#endif


#if defined(__cplusplus)
extern "C" {
#endif

struct jpeghdr {
        unsigned int tspec:8;   /* type-specific field */
        unsigned int off:24;    /* fragment byte offset */
        uint8_t	     type;      /* id of jpeg decoder params */
        uint8_t	     q;         /* quantization factor (or table id) */
        uint8_t      width;     /* frame width in 8 pixel blocks */
        uint8_t      height;    /* frame height in 8 pixel blocks */
};

struct jpeghdr_rst {
        uint16_t     dri;
        unsigned int f:1;
        unsigned int l:1;
        unsigned int count:14;
};

struct jpeghdr_qtable {
        uint8_t  mbz;
        uint8_t  precision;
        uint16_t length;
	uint8_t  lqt[64];
	uint8_t  cqt[64];
};

///////////// for manage streamer with web interface ///////
#define STOP_STREAM	0
#define START_STREAM	1
#define STOP_SCANNING	0
#define START_SCANNING	1

#define JPEG_ACCESS_MMAP 1
#define JPEG_ACCESS_DRV	 2

typedef struct strm_common {
	char 	address[21];   	// address or hostname
	int	port;		// port no
	double	fps;		// soft FPS
	int     regim;		// mmap/driver access to JPEG data
	int	scanning;	// mode scanning on/off	 
	char 	name[21];	// SDES name field for subtitles
	char	telephone[21];  // phone subtitles
	char	toolname[21];   
	char	note[61];
	char	location[41];
	char	email[21];
	char    encrypt_key[41];// encryption key
	int     encryption;	// mode encryption on/off
	int	status;		// start/top streamer
} strm_data_t;


///////////////// prototypes ///////////////////////////////////////

int 		usec_sleep(int);
Timestamp 	GetTimestamp(void);
Timestamp	GetTimerMs(void);
Timestamp 	GetRelativeTime(void);
void 		InitTimer(void);
void 		SeedRandom(void);
uint32_t 	GetRandomMcastAddress(void); 
uint16_t 	GetRandomPortBlock(void);
RtpTimestamp 	TimestampToRtp(Timestamp);
Timestamp 	TimestampToNtp(Timestamp);

int  getJPEGdata_mmap(int 		/* mmap file descriptor */, 
		      unsigned long * 	/* mmap memory */,
		      int 		/* size of mmap memory */,
		      int 		/* time interval between frames - 1/fps */, 
		      uint8_t  	*	/* JPEG data without header but with EOI*/,
		      uint64_t 	* 	/* time stamp current frame */
		);
int getFrame( uint8_t *, uint8_t *, uint8_t *, uint8_t *,
	    uint16_t *, uint16_t *, uint16_t *, uint8_t *);


#if defined(__cplusplus)
}
#endif

#endif

