/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
*/

#include <stdio.h>
#include <sys/types.h>
#include "system.h"

#include "output.h"

char *xmalloc();
char *xrealloc();
char *xstrdup();

int output_ctrl_init(output_ctrl_t *oc,
		     char *dirname, char *filename)
{
  /* Begin with 8 (output_t *):s
   * We are using an array of (output_t *) instead of just output_t since the
   * address of the output_t objects musn't change (they are referenced
   * directly in the stream objects).
   */
  oc->outputs=(output_t **)xmalloc(sizeof(output_t *)*8);

  oc->dirname=xstrdup(dirname);

  oc->basename=xstrdup(filename);

  /* FIXME: come up with a more portable solution */
  /* strip .ogg extension if it's there */
  if(strcasecmp((oc->basename+strlen(oc->basename)-4), ".ogg")==0){
    oc->basename[strlen(oc->basename)-4]='\0';
  }

  oc->idcount=0;
  oc->outputs_size=8;
  oc->outputs_used=0;

  return 1;
}

int output_ctrl_free(output_ctrl_t *oc)
{
  int i;
  for(i=0; i<oc->outputs_used; i++){
    fclose(oc->outputs[i]->file);
    free(oc->outputs[i]->filename);
    free(oc->outputs[i]);
  }

  free(oc->outputs);
  free(oc->dirname);
  free(oc->basename);

  return 1;
}

output_t *output_ctrl_output_new(output_ctrl_t *oc, int chain_c, int group_c)
{
  output_t *op;
  int fnlen, fnret;

  if(oc->outputs_used==oc->outputs_size){
    /* allocate room for 8 more (output_t *):s */
    oc->outputs=(output_t **)xrealloc(oc->outputs, sizeof(output_t *)*(oc->outputs_size+8));
    oc->outputs_size+=8;
  }

  op=(output_t *)xmalloc(sizeof(output_t));

  fnlen=strlen(oc->dirname)+strlen(oc->basename)+16;
  op->filename=xmalloc(fnlen);

  while(1){
    if(chain_c)
      if(group_c)
	fnret=snprintf(op->filename, fnlen, "%s%s.c%02d.g%02d.ogg",
		       oc->dirname, oc->basename, chain_c, group_c);
      else
	fnret=snprintf(op->filename, fnlen, "%s%s.c%02d.ogg",
		       oc->dirname, oc->basename, chain_c);
    else
      if(group_c)
	fnret=snprintf(op->filename, fnlen, "%s%s.g%02d.ogg",
		       oc->dirname, oc->basename, group_c);
      else
	fnret=snprintf(op->filename, fnlen, "%s%s.junk",
		       oc->dirname, oc->basename);

    if(fnret>=fnlen){
      /* try again and get it right this time */
      fnlen=fnret+1;
      op->filename=xrealloc(op->filename, fnlen);
      continue;
    }
    break;
  }

  op->file=fopen(op->filename, "w");
  if(op->file==NULL){
    fprintf(stderr, "Cannot open output file \"%s\": %s\n",
	    op->filename, strerror(errno));
    exit(1);
  }

  op->id=oc->idcount++;
  op->count=1;

  oc->outputs[oc->outputs_used++]=op;

  return op;
}

int output_ctrl_output_free(output_ctrl_t *oc, int id)
{
  int i;
  for(i=0; i<oc->outputs_used; i++){
    if(oc->outputs[i]->id==id){
      oc->outputs[i]->count--;
      if(oc->outputs[i]->count==0){
	fclose(oc->outputs[i]->file);
	free(oc->outputs[i]->filename);

	oc->outputs_used--;

	/* we may need to shift back the other (output_t *):s */
	if(oc->outputs_used > i)
	  memmove(&oc->outputs[i],
		  &oc->outputs[i+1],
		  sizeof(output_t *)*(oc->outputs_used-i));
      }
      return 1;
    }
  }
  return 0;
}

int output_page_write(output_t *op, ogg_page *og)
{
  if(fwrite(og->header, og->header_len, 1, op->file)==0)return 0;
  if(fwrite(og->body, og->body_len, 1, op->file)==0)return 0;

  return 1;
}
