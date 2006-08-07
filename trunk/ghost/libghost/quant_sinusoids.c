/**
   @file sinusoids.c
   @brief Sinusoid extraction/synthesis
*/

/* Copyright (C) 2005

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include <math.h>
#include "sinusoids.h"
#include <stdio.h>
#include "ghost.h"

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))




/* Quantizes the extracted sinusoids 
 * w are the frequencies in current frame
 * N is the number of sinusoids
 * SineStruct is structure holding quantized sine params
*/

void quantize_sinusoids(float *wi,int N,QuantSine SineStruct[SINUSOIDS])
{

/* sort the current sinusoids */
	float w[SINUSOIDS];
	int i,j,lindex = 0;
	static QuantSine prevSineStruct[SINUSOIDS];
	

	for(i=0;i<N;i++)
	{
		w[i] = wi[i];
	}

	for(i=0;i<N;i++)
	{
		for(j=(i+1);j<N;j++)
		{
			if(w[j]< w[i])
			{
				float t = w[i];
				w[i] = w[j];
				w[j] = t;
			}
		}
	}

	/* start with lowest freq and group its harmonics. Go on till
all sinuosids belong to a group*/

	for(i=0;i<N;i++)
	{
		if(w[i]!= -1)
		{
			SineStruct[lindex].basefreq = w[i];
			SineStruct[lindex].harmonicstate;
			for(j=(i+1);j<N;j++)
			{
				if(w[j]!= -1)
				{
					int harmonic = w[j]/SineStruct[lindex].basefreq;
					
					if((w[j]/SineStruct[lindex].basefreq - (float)(harmonic) < 0.01)&&
						(w[j]/SineStruct[lindex].basefreq - (float)(harmonic) > -0.01))
					{
						w[j] = -1;
						SineStruct[lindex].harmonicstate = (SineStruct[lindex].harmonicstate)
															| (1<<harmonic);
					}

				}
			}
			lindex++;
		}
	}


/* compare the current groups with groups of prev frames 
and see which all have to be transmitted */  
	i= 0;
	while(i<lindex)
	{
		/* compare the current set with all the freq in the prev bands*/
		if(SineStruct[i].basefreq < prevSineStruct[j].basefreq)
		{
			//add i
			i++;
		}
		else if(SineStruct[i].basefreq > prevSineStruct[j].basefreq)
		{
			//rem j
			j++;
		}
		else
		{
			if(SineStruct[i].harmonicstate != prevSineStruct[j].harmonicstate)//state[i] != prevstate [j]
			{
				//update state of i 
			}
		}
		
	}
}

/* Inverse quantizes the extracted sinusoids 
 * w are the frequencies in current frame
 * N is the number of sinusoids
 * len is the frame size
*/

void invquantize_sinusoids(float *x,float *w,int N,int len)
{

/*add the sinuosids newly sent in current frame to wi vector */

	/* check for all teh sinusoids; add the ones which are to be added and remove the ones which are to be removed to the 
	w vector already existing */
	
}

/* Synthesizes the signal from the sinusoids 
 * w are the frequencies
 * ai are the cos(x) coefficients
 * bi are the sin(x) coefficients
 * y is the synthesized signal by summing all the params
 * N is the number of sinusoids
 * len is the frame size
*/
void generate_sinusoids(float *w,
						float *window,
						float *ai,
						float *bi,
						float *y,
						int N,
						int len)
{
	int i,j;
	float cos_table[SINUSOIDS][LENGTH];
    float sin_table[SINUSOIDS][LENGTH];

	for (i=0;i<N;i++)
    {
      float tmp1=0, tmp2=0;
      float tmp3=0, tmp4=0;
      for (j=0;j<len;j++)
      {
         cos_table[i][j] = cos(w[i]*j)*window[j];
         sin_table[i][j] = sin(w[i]*j)*window[j];
      }
    }

	for (i=0;i<N;i++)
    {
         for (j=0;j<len;j++)
         {
            y[j] += ai[i]*cos_table[i][j] + bi[i]*sin_table[i][j];
         }         
    }

}
