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

#ifndef _OUTPUT_H
#define _OUTPUT_H

#include <stdio.h>
#include <ogg/ogg.h>

typedef struct {
  FILE *file;
  char *filename;

  int id; /* unique number for each output_t */
  int count; /* the number of streams writing to the file */
} output_t;

typedef struct {
  output_t **outputs;
  int outputs_size;
  int outputs_used;
  int idcount;

  char *basename;
  char *suffix;
} output_ctrl_t;


int       output_ctrl_init(output_ctrl_t *oc, const char *pathname);
int       output_ctrl_free(output_ctrl_t *oc);
output_t *output_ctrl_output_new(output_ctrl_t *oc, int chain_c, int group_c, int dryrun);
int       output_ctrl_output_free(output_ctrl_t *oc, int id);

int       output_page_write(output_t *op, ogg_page *og);

#endif /* _OUTPUT_H */
