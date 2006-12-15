/*
 *
 *     sushivision copyright (C) 2006 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <stdio.h>
#include <math.h>
#include "sushivision.h"

sushiv_instance_t *s;
#define todB(x)   ((x)==0?-400.f:log((x)*(x))*4.34294480f)

/* time, blocksz, amp_0, amp_del, freq_0, phase_0, phase_del */
int funcsize=64;
double function[64];
static double fourier_objective(double *d){
  int i;
  double re_obj=0;
  double im_obj=0;
  double obj;

  for(i=0;i<funcsize;i++){
    im_obj += function[i] * sin(i*d[4]*2*M_PI);
    re_obj += function[i] * cos(i*d[4]*2*M_PI);
  
  }
  im_obj/=funcsize;
  re_obj/=funcsize;

  obj = sqrt(im_obj*im_obj + re_obj*re_obj);
  return todB(obj);

}

/*
static double objective(double *d){
  
  return .5;

}
*/

static int time_callback(sushiv_dimension_t *d){


  return 1; // indicate that default processing chain should continue
}

static int blocksize_callback(sushiv_dimension_t *d){


  return 1; // indicate that default processing chain should continue
}

int sushiv_submain(int argc, char *argv[]){
  int i;

  {
    double phasechirp = 0;
    double phi = 0;

    for(i=0;i<funcsize;i++){
      phasechirp +=.005;
      phi += .1+phasechirp;
      
      function[i]=sin(phi)*.1;
    }
  }

  s=sushiv_new_instance();

  sushiv_new_dimension(s,0,"time",
		       4,(double []){0,1024,2048,4096},
		       time_callback,
		       SUSHIV_NO_X|SUSHIV_NO_Y);
  sushiv_new_dimension(s,1,"blocksize",
		       8,(double []){64,128,256,512,1024,2048,4096,8192},
		       blocksize_callback,
		       SUSHIV_NO_X|SUSHIV_NO_Y);

  sushiv_new_dimension(s,2,"amplitude",
		       9,(double []){-96,-84,-72,-60,-48,-36,-24,-12,0},
		       NULL,0);
  sushiv_new_dimension(s,3,"amplitude delta",
		       9,(double []){-96,-48,-24,-12,0,12,24,48,96},
		       NULL,0);
  sushiv_new_dimension(s,4,"frequency",
		       6,(double []){0,.1,.2,.3,.4,.5},
		       NULL,0);
  sushiv_new_dimension(s,5,"phase",
		       3,(double []){-.5,0,.5},
		       NULL,0);
  sushiv_new_dimension(s,6,"phase delta",
		       3,(double []){-10,0,10},
		       NULL,0);
  
  sushiv_new_objective(s,0,"fourier",
		       8,(double []){-96,-48,-36,-24,-12,-6,0,6},
		       fourier_objective,0);
  //sushiv_new_objective(s,1,"fit",fit_objective,0);
  //sushiv_new_objective(s,2,"waveform",fourier_objective,0);

  sushiv_new_panel_2d(s,0,"fourier objective",
		      (int []){0,-1},
		      (int []){2,3,4,5,6,-1},
		      0);
  
  /*sushiv_linked_panel_1d(s,1,"fourier x slice",8,
		      (double []){-96,-48,-36,-24,-12,-6,0,6},
		      0,0,
		      0);
  sushiv_linked_panel_1d(s,2,"fourier y slice",8,
		      (double []){-96,-48,-36,-24,-12,-6,0,6},
		      0,1,
		      0);
  sushiv_new_panel_1d(s,3,"input block",8,
		      (double []){-96,-48,-36,-24,-12,-6,0,6},
		      (int *){2,-1},
		      NULL,
		      0);*/

  return 0;
}
