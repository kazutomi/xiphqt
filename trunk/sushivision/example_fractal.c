/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
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

#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include "sushivision.h"

static void fractal_objective(double *d, double *ret){
  int max_iter = d[4];
  int i;
  double z, zi, zz;
  double c=d[0],ci=d[1];
  
  ret[0]=NAN;
  ret[1]=1.;

  z = d[2]; zi = d[3];
  for(i=0;i<max_iter;i++){
    zz = z*z - zi*zi + c;
    zi = 2.0*z*zi + ci;
    z  = zz;
    if (z*z + zi*zi > 4.0){
      ret[0] = (double)i/max_iter;
      ret[1] = NAN;
      return;
    }
  }

  ret[1] = sqrt(z*z + zi*zi)/4.;
}

int main(int argc, char *argv[]){

  // before any gtk, gdk or glib setup
  sv_init();
  
  //g_thread_init (NULL);
  //gtk_init (&argc, &argv);
  //gdk_threads_init ();

  // "name:label(arg,arg,arg...)"

  sv_dim_new("rc:Re\\(c\\)");
  sv_dim_scale("-2.25, -0.75, 0, 0.25, 0.75");
  
  sv_dim_new("ic:Im\\(c\\)");
  sv_dim_scale("-2,-1,0,1,2");

  sv_dim_new("rz:Re\\(z0\\)");
  sv_dim_scale("-2.25, -1, 0, 1, 2.25");

  sv_dim_new("iz:Im\\(z0\\)");
  sv_dim_scale("-2.25, -1, 0, 1, 2.25");

  sv_dim_new("it:Max Iterations(picklist)");
  sv_dim_scale("100:one hundred,"
	       "1000:one thousand,"
	       "10000:ten thousand,"
	       "100000:one hundred thousand");
  sv_dim_value(10000);

  sv_obj_new("fractal",fractal_objective,"rc,ic,rz,iz,it", "outer,inner");
  sv_obj_scale("outer","0, .001, .01, .1, 1.0");
  sv_obj_scale("inner","0, .001, .01, .1, 1.0");
  
  sv_panel_new("Mandel/Julia Fractal",
	       "fractal(2d,Z=inner), fractal(2d)",
	       "rc(X,Y), ic(X,Y), rz(X,Y), iz(X,Y), it");
  
  sv_go();
  sv_join();

  return 0;
}
