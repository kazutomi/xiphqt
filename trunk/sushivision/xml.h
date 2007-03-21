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

extern xmlNodePtr _xmlGetChildS(xmlNodePtr n, char *name,char *prop, char *val);
extern xmlNodePtr _xmlGetChildI(xmlNodePtr n, char *name,char *prop, int val);
extern void _xmlNewMapProp(xmlNodePtr n, char *name, _sv_propmap_t **map, int val);
extern void _xmlNewPropF(xmlNodePtr n, char *name, double val);
extern void _xmlNewPropI(xmlNodePtr n, char *name, int val);
extern void _xmlNewPropS(xmlNodePtr n, char *name, char *val);

extern void _xmlCheckPropS(xmlNodePtr n, char *prop, char *val, char *msg, int num, int *warn);
extern void _xmlCheckMap(xmlNodePtr n, char *prop, _sv_propmap_t **map, int val, char *msg, int num, int *warn);

extern void _xmlGetPropS(xmlNodePtr n, char *name, char **out);
extern void _xmlGetPropF(xmlNodePtr n, char *name, double *out);
extern void _xmlGetChildMap(xmlNodePtr in, char *prop, char *key, _sv_propmap_t **map, int *out,
			  char *msg, int num, int *warn);
extern void _xmlGetChildMapPreserve(xmlNodePtr in, char *prop, char *key, _sv_propmap_t **map, int *out,
				   char *msg, int num, int *warn);
extern void _xmlGetChildPropS(xmlNodePtr in, char *prop, char *key, char **out);
extern void _xmlGetChildPropSPreserve(xmlNodePtr in, char *prop, char *key, char **out);
extern void _xmlGetChildPropF(xmlNodePtr in, char *prop, char *key, double *out);
extern void _xmlGetChildPropFPreserve(xmlNodePtr in, char *prop, char *key, double *out);
extern void _xmlGetChildPropI(xmlNodePtr in, char *prop, char *key, int *out);
extern void _xmlGetChildPropIPreserve(xmlNodePtr in, char *prop, char *key, int *out);

extern int _sv_propmap_pos(_sv_propmap_t **map, int val);
extern int _sv_propmap_last(_sv_propmap_t **map);
extern int _sv_propmap_label_pos(_sv_propmap_t **map, char *label);
