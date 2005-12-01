/******************************************************************************
This is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	
******************************************************************************/
// Streamer for Elphel JPEG camera 313 input device
// Copyright (c) 2004 Andy Latin aka Landy, Dmitry Latin aka ipfreely
// All rights reserved.
//
// http://andy.qnx.org.ru
// mail to:  
// andy@qnx.org.ru   
// landy@mail333.com
/////////////////////////////////////////////////////////////////////////////


#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "defines.h"


///////////////// Time functions ///////////////////////////////////////

int usec_sleep(int usec_delay)
{
#ifdef HAVE_NANOSLEEP
    struct timespec ts;
    ts.tv_sec  =  usec_delay / TimestampTicks;
    ts.tv_nsec = (usec_delay % TimestampTicks) * 1000;
    return nanosleep(&ts, NULL);
#else
    return usleep(usec_delay);
#endif
}

// Returns current time in microseconds
Timestamp GetTimestamp()
{
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec * TimestampTicks) + tv.tv_usec;
}

// Returns current time in milliseconds
Timestamp GetTimerMs(void)
{
     return GetTimestamp()/1000;
}  


static Timestamp RelativeTime=0;

// Returns time spent between now and last call in microseconds
Timestamp GetRelativeTime()
{
  Timestamp t,r;

  t=GetTimestamp();
  r=t-RelativeTime;
  RelativeTime=t;
  return r;
}

// Initialize timer, must be called at least once at start
void InitTimer()
{
  GetRelativeTime();
}


static  int once = 0;

void SeedRandom(void)
{
   if (!once) {
	srandom(time(NULL));
	once = 1;
   }
}

uint32_t GetRandomMcastAddress(void) 
{
	SeedRandom();

	// pick a random number in the multicast range
	uint32_t mcast = ((random() & 0x0FFFFFFF) | 0xE0000000);

	// screen out undesirable values
	// introduces small biases in the results

	// stay away from 224.0.0.x
	if ((mcast & 0x0FFFFF00) == 0) {
		mcast |= 0x00000100;	// move to 224.0.1
	} 

	// stay out of SSM range 232.x.x.x
	// user should explictly select this if they want SSM
	if ((mcast & 0xFF000000) == 232) {
		mcast |= 0x01000000;	// move to 233
	}

	// stay away from .0 .1 and .255
	if ((mcast & 0xFF) == 0 || (mcast & 0xFF) == 1 
	  || (mcast & 0xFF) == 255) {
		mcast = (mcast & 0xFFFFFFF0) | 0x4;	// move to .4 or .244
	}

  return htonl(mcast);
}

uint16_t GetRandomPortBlock(void) 
{
	SeedRandom();

	// Get random block of 4 port numbers above 20000
	return (uint16_t)(20000 + ((random() >> 18) << 2));
}


///////////////////// Rtp time functions /////////////////////

static Timestamp StartTimestamp = 0;
static Timestamp RtpTimestampOffset =0;

RtpTimestamp TimestampToRtp(Timestamp t)
{
  if (StartTimestamp !=0) {
	    return (uint32_t)(((t - StartTimestamp) 
		* TimeScale) / TimestampTicks)
	      	+ RtpTimestampOffset;
	  }
	  StartTimestamp = t;
	  return 0;
	}

static const u_int32_t SECS_BETWEEN_1900_1970 = 2208988800U;

Timestamp TimestampToNtp(Timestamp t) 
{
	// low order ntp 32 bits is 2 ^ 32 -1 ticks per sec
	register uint32_t usecs = t % TimestampTicks;
	return (((t / TimestampTicks) + SECS_BETWEEN_1900_1970) << 32)
		| ((usecs << 12) + (usecs << 8) - ((usecs * 3650) >> 6));
}

void HexDump(
        u_int8_t* pBytes, u_int32_t numBytes)
{
	uint32_t i;

        printf("<%u bytes> ", numBytes);
        for (i = 0; i < numBytes; i++) {
                if ((i % 16) == 0 && numBytes > 16) {
                        printf("\n");
                }
                printf("%02x ", pBytes[i]);
        }
        printf("\n");
}
#if 0
void main(){
  float t=0;
  InitTimer();
  while(1){ t+=GetRelativeTime();printf("time= %10.6f\r",t);fflush(stdout); }
}
#endif

