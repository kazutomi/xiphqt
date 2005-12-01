// Implementation of Theora RTP streaming
// Copyright 2005 Elphel, Inc
// Andrey Latin andrey.latin@gmail.com

#ifndef __STRAMER_H_DEF_
#define __STREAMER_H_DEF_

#include <inttypes.h>


#define HAVE_NANOSLEEP 	1

#define RTP_PT_THEORA	98
#define BUF_SIZE	1460
#define TimeScale	90000

#define TimestampTicks	1000000

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

#define RAW_TH_DATA	0
#define TH_IDENT_HDR	0x80
#define TH_COMMENT_HDR	0x81
#define TH_SETUP_HDR	0x82


// Payload RTP header for Theora bitstream

struct TheoraPayloadHeader {
	uint32_t	shident:32;	// Setup Header Ident
	uint8_t		C_bit:1;
	uint8_t		F_bit:1;
	uint8_t		reserv:2;
	uint8_t		n_pkts:4;	// number of complete packets in payload
	uint8_t		type_payload:8;
	uint16_t	payload_len;
};

///////////// for manage streamer with web interface ///////
#define STOP_STREAM	0
#define START_STREAM	1
//#define STOP_SCANNING	0
//#define START_SCANNING	1

//#define JPEG_ACCESS_MMAP 1
//#define JPEG_ACCESS_DRV	 2

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
void HexDump(
        u_int8_t* pBytes, u_int32_t numBytes);




#if defined(__cplusplus)
}
#endif

#endif //__STRAMER_H_DEF_

