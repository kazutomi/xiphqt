/* 
 * oggsplit - splits multiplexed Ogg files into separate files
 *
 * Copyright (C) 2003 Philip Jägenstedt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "output.h"
#include "common.h"

/* outdir given on command line */
extern char *outdir;

int output_ctrl_init(output_ctrl_t *oc, const char *pathname)
{
  int pathname_len, i;
  /* indexes in pathname where basname begs ends ends.*/
  int bnb=-1, bne=-1;
  int basename_len;

  /* Begin with 8 (output_t *):s
   * We are using an array of (output_t *) instead of just output_t since the
   * address of the output_t objects musn't change (they are referenced
   * directly in the stream objects).
   */
  oc->outputs=xmalloc(sizeof(output_t *)*8);

  oc->idcount=0;
  oc->outputs_size=8;
  oc->outputs_used=0;

  oc->basename=NULL;
  oc->suffix=NULL;

  pathname_len=strlen(pathname);

  /* calc bnb */
  for(i=pathname_len; i>0; i--){
    if(pathname[i-1]=='/'){
      bnb=i;
      break;
    }
  }
  if(bnb==-1)bnb=0;

  /* calc bne */
  for(i=pathname_len-1; i>pathname_len-6 && i>bnb; i--){
    if(pathname[i]=='.'){
      bne=i;
      break;
    }
  }

  if(bne==-1)
    basename_len=pathname_len-bnb;
  else
    basename_len=bne-bnb;

  if(outdir!=NULL){
    int outdirlen=strlen(outdir);
    oc->basename=xmalloc(outdirlen+basename_len+2);
    strcpy(oc->basename, outdir);
    oc->basename[outdirlen]='/';
    strncpy(&oc->basename[outdirlen+1], &pathname[bnb], basename_len);
    oc->basename[outdirlen+basename_len+2]='\0';
  }else{
    oc->basename=xmalloc(basename_len+1);
    strncpy(oc->basename, &pathname[bnb], basename_len);
    oc->basename[basename_len]='\0';
  }

  if(bne==-1)
    oc->suffix=xstrdup(".ogg");
  else
    oc->suffix=xstrdup(&pathname[bne]);

  return 1;
}

int output_ctrl_free(output_ctrl_t *oc)
{
  int i;
  for(i=0; i<oc->outputs_used; i++){
    if (oc->outputs[i]->file) fclose(oc->outputs[i]->file);
    free(oc->outputs[i]->filename);
    free(oc->outputs[i]);
  }

  free(oc->outputs);
  free(oc->basename);

  return 1;
}

output_t *output_ctrl_output_new(output_ctrl_t *oc, int chain_c, int group_c, int dryrun)
{
  output_t *op;
  int fnlen, fnret;

  if(oc->outputs_used==oc->outputs_size){
    /* allocate room for 8 more (output_t *):s */
    oc->outputs=xrealloc(oc->outputs, sizeof(output_t *)*(oc->outputs_size+8));
    oc->outputs_size+=8;
  }

  op=xmalloc(sizeof(output_t));

  fnlen=strlen(oc->basename)+strlen(oc->suffix)+16;
  op->filename=xmalloc(fnlen);

  while(1){
    if(chain_c)
      if(group_c)
	fnret=snprintf(op->filename, fnlen, "%s.c%02d.g%02d%s",
		       oc->basename, chain_c, group_c, oc->suffix);
      else
	fnret=snprintf(op->filename, fnlen, "%s.c%02d%s",
		       oc->basename, chain_c, oc->suffix);
    else
      if(group_c)
	fnret=snprintf(op->filename, fnlen, "%s.g%02d%s",
		       oc->basename, group_c, oc->suffix);
      else
	fnret=snprintf(op->filename, fnlen, "%s.junk%s",
		       oc->basename, oc->suffix);

    if(fnret>=fnlen){
      /* try again and get it right this time */
      fnlen=fnret+1;
      op->filename=xrealloc(op->filename, fnlen);
      continue;
    }
    break;
  }

  if (!dryrun){
    op->file=fopen(op->filename, "w");
    if(op->file==NULL){
      fprintf(stderr, "Cannot open output file \"%s\": %s\n",
	      op->filename, strerror(errno));
      exit(1);
    }
  }
  else
    op->file=NULL;

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
        if (oc->outputs[i]->file) fclose(oc->outputs[i]->file);
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
  if (op->file){
    if(fwrite(og->header, og->header_len, 1, op->file)==0)return 0;
    if(fwrite(og->body, og->body_len, 1, op->file)==0)return 0;
  }

  return 1;
}
