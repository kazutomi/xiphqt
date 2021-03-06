/* OggEnc
 **
 ** This program is distributed under the GNU General Public License, version 2.
 ** A copy of this license is included with this source.
 **
 ** Copyright 2000, Michael Smith <msmith@xiph.org>
 **
 ** Portions from Vorbize, (c) Kenneth Arnold <kcarnold@yahoo.com>
 ** and libvorbis examples, (c) Monty <monty@xiph.org>
 **/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <vorbis/vorbisenc.h>
#include "encode.h"


#define READSIZE 1024

extern float nominalBitrate;
extern char  approxBRCaption[];

int oe_write_page(ogg_page *page, FILE *fp);

int oe_encode(oe_enc_opt *opt)
{

	ogg_stream_state os;
	ogg_page 		 og;
	ogg_packet 		 op;

	vorbis_dsp_state vd;
	vorbis_block     vb;
	vorbis_info      vi;

	long samplesdone=0;
    int eos;
	long bytes_written = 0, packetsdone=0;
	double time_elapsed;
	int ret=0;

	TIMER *timer;


	/* get start time. */
	timer = timer_start();

	/* Have vorbisenc choose a mode for us */
	vorbis_info_init(&vi);
  
	if (opt->oeMode == OE_MODE_BITRATE)
  {
		if (vorbis_encode_init(&vi, opt->channels, opt->rate, -1, 
			opt->bitrate*1000, -1))
    {
      opt->error("Can't encode with selected params/file attrs");
      vorbis_info_clear(&vi);
      return 1;
    }
    else
    {
      (void) strcpy(approxBRCaption, "Bitrate");
      nominalBitrate = (float)opt->bitrate*1000;
    }
  }
	else
  {
	  if (vorbis_encode_init_vbr(&vi, opt->channels, opt->rate, 
                        opt->quality_coefficient))
    {
      opt->error("Can't encode with selected params/file attrs");
      vorbis_info_clear(&vi);
      return 1;
    }
    else
    {
      (void) strcpy(approxBRCaption, "Nominal Bitrate");
      nominalBitrate = (float)vi.bitrate_nominal;
    }
  }

	/* Now, set up the analysis engine, stream encoder, and other
	   preparation before the encoding begins.
	 */

	vorbis_analysis_init(&vd,&vi);
	vorbis_block_init(&vd,&vb);

	ogg_stream_init(&os, opt->serialno);

	/* Now, build the three header packets and send through to the stream 
	   output stage (but defer actual file output until the main encode loop) */

	{
		ogg_packet header_main;
		ogg_packet header_comments;
		ogg_packet header_codebooks;
		int result;

		/* Build the packets */
		vorbis_analysis_headerout(&vd,opt->comments,
				&header_main,&header_comments,&header_codebooks);

		/* And stream them out */
		ogg_stream_packetin(&os,&header_main);
		ogg_stream_packetin(&os,&header_comments);
		ogg_stream_packetin(&os,&header_codebooks);

		while((result = ogg_stream_flush(&os, &og)))
		{
			if(!result) break;
			ret = oe_write_page(&og, opt->out);
			if(!ret)
			{
				opt->error("Failed writing header to output stream\n");
				ret = 1;
				goto cleanup; /* Bail and try to clean up stuff */
			}
			else
				bytes_written += ret;
		}
	}

	eos = 0;

	/* Main encode loop - continue until end of file */
	while(!eos)
	{
		float **buffer = vorbis_analysis_buffer(&vd, READSIZE);
		long samples_read = opt->read_samples(opt->readdata, 
				buffer, READSIZE);

		if(samples_read ==0)
			/* Tell the library that we wrote 0 bytes - signalling the end */
			vorbis_analysis_wrote(&vd,0);
		else
		{
			samplesdone += samples_read;

			/* Call progress update every 10 samples */			
      if (samplesdone % 10 == 0)
			{
				double time;

				time = timer_time(timer);

				opt->progress_update(opt->filename, opt->total_samples_per_channel, 
						samplesdone, time);
			}

			/* Tell the library how many samples (per channel) we wrote 
			   into the supplied buffer */
			vorbis_analysis_wrote(&vd, samples_read);
		}

		/* While we can get enough data from the library to analyse, one
		   block at a time... */
		while(vorbis_analysis_blockout(&vd,&vb)==1)
		{

			/* Do the main analysis, creating a packet */
      vorbis_analysis(&vb,NULL);	
			vorbis_bitrate_addblock(&vb);

			while(vorbis_bitrate_flushpacket(&vd, &op)) 
			{

				/* Add packet to bitstream */
				ogg_stream_packetin(&os,&op);
				packetsdone++;

				/* If we've gone over a page boundary, we can do actual output,
				   so do so (for however many pages are available) */

				while(!eos)
				{
					int result = ogg_stream_pageout(&os,&og);
					if(!result) break;

					ret = oe_write_page(&og, opt->out);
					if(!ret)
					{
						opt->error("Failed writing data to output stream\n");
						ret = 1;
						goto cleanup; /* Bail */
					}
					else
						bytes_written += ret; 

					if(ogg_page_eos(&og))
						eos = 1;
				}
			}
		}
	}

  if (nominalBitrate <= 0.0)
  {
    nominalBitrate = (float)(8.0*((double)bytes_written
    / ((double)samplesdone/(double)opt->rate)));

    (void) strcpy(approxBRCaption, "Average Bitrate");
  }

	/* Cleanup time */
cleanup:

	ogg_stream_clear(&os);

	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_info_clear(&vi);

	time_elapsed = timer_time(timer);
	opt->end_encode(opt->filename, time_elapsed, opt->rate, samplesdone, bytes_written);

	timer_clear(timer);

	return ret;
}

int oe_write_page(ogg_page *page, FILE *fp)
{
	int written;
	written = fwrite(page->header,1,page->header_len, fp);
	written += fwrite(page->body,1,page->body_len, fp);

	return written;
}
