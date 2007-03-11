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

extern xmlNodePtr xmlGetChildS(xmlNodePtr n, char *name,char *prop, char *val);
extern xmlNodePtr xmlGetChildI(xmlNodePtr n, char *name,char *prop, int val);
extern void xmlNewMapProp(xmlNodePtr n, char *name, propmap **map, int val);
extern void xmlNewPropF(xmlNodePtr n, char *name, double val);
extern void xmlNewPropI(xmlNodePtr n, char *name, int val);
extern void xmlNewPropS(xmlNodePtr n, char *name, char *val);

extern void xmlCheckPropS(xmlNodePtr n, char *prop, char *val, char *msg, int num, int *warn);
extern void xmlCheckMap(xmlNodePtr n, char *prop, propmap **map, int val, char *msg, int num, int *warn);

extern void xmlGetPropS(xmlNodePtr n, char *name, char **out);
extern void xmlGetPropF(xmlNodePtr n, char *name, double *out);
extern void xmlGetChildMap(xmlNodePtr in, char *prop, char *key, propmap **map, int *out,
			  char *msg, int num, int *warn);
extern void xmlGetChildPropS(xmlNodePtr in, char *prop, char *key, char **out);
extern void xmlGetChildPropSPreserve(xmlNodePtr in, char *prop, char *key, char **out);
extern void xmlGetChildPropF(xmlNodePtr in, char *prop, char *key, double *out);
extern void xmlGetChildPropFPreserve(xmlNodePtr in, char *prop, char *key, double *out);
extern void xmlGetChildPropI(xmlNodePtr in, char *prop, char *key, int *out);
extern void xmlGetChildPropIPreserve(xmlNodePtr in, char *prop, char *key, int *out);

extern int propmap_pos(propmap **map, int val);
extern int propmap_last(propmap **map);
extern int propmap_label_pos(propmap **map, char *label);
