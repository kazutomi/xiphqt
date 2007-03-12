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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "internal.h"

/* a few helpers to make specific libxml2 call patterns more concise */

xmlNodePtr xmlGetChildSPreserve(xmlNodePtr n, char *name,char *prop, char *val){
  xmlNodePtr child = (n?n->xmlChildrenNode:NULL);
  while(child){
    // is this the child we want?
    if (child->type == XML_ELEMENT_NODE && !xmlStrcmp(child->name, (const xmlChar *)name)) {
      // does the desired attribut match?
      xmlChar *test = (prop?xmlGetProp(child, (const xmlChar *)prop):NULL);
      if(!prop || test){
	if (!prop || !xmlStrcmp(test, (const xmlChar *)val)) {
	  if(test)
	    xmlFree(test);

	  // return it
	  return child;
	}
	if(test)
	  xmlFree(test);
      }
    }
    child = child->next;
  }
  return NULL;
}

xmlNodePtr xmlGetChildS(xmlNodePtr n, char *name,char *prop, char *val){
  xmlNodePtr child = (n?xmlGetChildSPreserve(n,name,prop,val):NULL);
  if(child){
    // this is what we want 
    // remove it from the tree
    xmlUnlinkNode(child);
  }

  return child;
}

xmlNodePtr xmlGetChildI(xmlNodePtr n, char *name,char *prop, int val){
  char buffer[80];
  snprintf(buffer,sizeof(buffer),"%d",val);
  return xmlGetChildS(n, name, prop, buffer);
}

xmlNodePtr xmlGetChildIPreserve(xmlNodePtr n, char *name,char *prop, int val){
  char buffer[80];
  snprintf(buffer,sizeof(buffer),"%d",val);
  return xmlGetChildSPreserve(n, name, prop, buffer);
}

void xmlNewMapProp(xmlNodePtr n, char *name, propmap **map, int val){
  propmap *m = *map++;
  while(m){
    if(m->value == val){
      xmlNewProp(n, (xmlChar *)name, (xmlChar *)m->left);
      break;
    }
    m = *map++;
  }
}

void xmlNewPropF(xmlNodePtr n, char *name, double val){
  char buffer[80];
  snprintf(buffer,sizeof(buffer),"%.20g",val);
  xmlNewProp(n, (xmlChar *) name, (xmlChar *)buffer);
}

void xmlNewPropI(xmlNodePtr n, char *name, int val){
  char buffer[80];
  snprintf(buffer,sizeof(buffer),"%d",val);
  xmlNewProp(n, (xmlChar *) name, (xmlChar *)buffer);
}

void xmlNewPropS(xmlNodePtr n, char *name, char *val){
  if(val)
    xmlNewProp(n, (xmlChar *) name, (xmlChar *)val);
}

void xmlGetPropS(xmlNodePtr n, char *name, char **value){
  char *ret = (char *)(n?xmlGetProp(n, (xmlChar *)name):NULL);
  if(ret) *value=ret;
}

void xmlGetPropF(xmlNodePtr n, char *name, double *val){
  xmlChar *v = (n?xmlGetProp(n, (xmlChar *)name):NULL);
  if(v){
    *val = atof((char *)v);
    xmlFree(v);
  }
}

void xmlCheckPropS(xmlNodePtr n, char *prop, char *val, char *msg, int num, int *warn){
  char *testval = NULL;
  xmlGetPropS(n, prop, &testval);
  if(testval){
    if(strcmp(val,testval)){
      first_load_warning(warn);
      fprintf(stderr,msg, num);
      fprintf(stderr,"\n\t(found %s, should be %s).\n", testval, val);
    }
    xmlFree(testval);
  }else{
    first_load_warning(warn);
    fprintf(stderr,msg, num);
    fprintf(stderr,"\n\t(null found, should be %s).\n", val);
  }
}

void xmlCheckMap(xmlNodePtr n, char *prop, propmap **map, int val, char *msg, int num, int *warn){
  char *name = NULL;
  char *testname = NULL;
  xmlGetPropS(n, prop, &testname);
  
  // look up our desired value
  propmap *m = *map++;
  while(m){
    if(m->value == val){
      name = m->left;
      break;
    }
    m = *map++;
  }

  if(testname){
    if(name){
      if(strcmp(name,testname)){
	first_load_warning(warn);
	fprintf(stderr,msg, num);
	fprintf(stderr,"\n\t(found %s, should be %s).\n", testname, name);
      }
    }else{
      first_load_warning(warn);
      fprintf(stderr,msg, num);
      fprintf(stderr,"\n\t(found %s, should be null).\n", testname);
    }
    xmlFree(testname);
  }else{
    if(name){
      first_load_warning(warn);
      fprintf(stderr,msg, num);
      fprintf(stderr,"\n\t(null found, should be %s).\n", name);
    }
  }
}

static void xmlGetMapVal(xmlNodePtr n, char *key, propmap **map, int *out){
  char *valname = (char *)(n?xmlGetProp(n, (xmlChar *)key):NULL);
  if(!valname) return;
  
  propmap *m = *map++;
  while(m){
    if(!strcmp(m->left,valname)){
      xmlFree(valname);
      *out = m->value;
      return;
    }
    m = *map++;
  }
  xmlFree(valname);
}

static void xmlGetChildMap_i(xmlNodePtr in, char *prop, char *key, propmap **map, int *out,
			     char *msg, int num, int *warn, int pres){
  xmlNodePtr n = (pres ?
		  (in?xmlGetChildSPreserve(in, prop, NULL, NULL):NULL):
		  (in?xmlGetChildS(in, prop, NULL, NULL):NULL));
  if(!n)return;
  
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  if(!val){
    if(!pres) xmlFreeNode(n);
    return;
  }

  int ret = -1;
  xmlGetMapVal(n,key,map,&ret);
  if(ret == -1){
    if(msg){
      first_load_warning(warn);
      fprintf(stderr,msg,num);
      fprintf(stderr," (%s).\n", val);
    }
  }else
    *out = ret;

  xmlFree(val);
  if(!pres) xmlFreeNode(n);
}

void xmlGetChildMap(xmlNodePtr in, char *prop, char *key, propmap **map, int *out,
		    char *msg, int num, int *warn){
  xmlGetChildMap_i(in, prop, key, map, out, msg, num, warn, 0);
}

void xmlGetChildMapPreserve(xmlNodePtr in, char *prop, char *key, propmap **map, int *out,
		    char *msg, int num, int *warn){
  xmlGetChildMap_i(in, prop, key, map, out, msg, num, warn, 1);
}

void xmlGetChildPropS(xmlNodePtr in, char *prop, char *key, char **out){
  xmlNodePtr n = (in?xmlGetChildS(in, prop, NULL, NULL):NULL);
  if(!n)return;
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  xmlFreeNode(n);
  if(val) *out = val;
}

void xmlGetChildPropSPreserve(xmlNodePtr in, char *prop, char *key, char **out){
  xmlNodePtr n = (in?xmlGetChildSPreserve(in, prop, NULL, NULL):NULL);
  if(!n)return;
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  if(val) *out = val;
}

void xmlGetChildPropF(xmlNodePtr in, char *prop, char *key, double *out){
  xmlNodePtr n = (in?xmlGetChildS(in, prop, NULL, NULL):NULL);
  if(!n)return;
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  xmlFreeNode(n);
  if(val) *out = atof(val);
  xmlFree(val);
}
  
void xmlGetChildPropFPreserve(xmlNodePtr in, char *prop, char *key, double *out){
  xmlNodePtr n = (in?xmlGetChildSPreserve(in, prop, NULL, NULL):NULL);
  if(!n)return;
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  if(val) *out = atof(val);
  xmlFree(val);
}

void xmlGetChildPropI(xmlNodePtr in, char *prop, char *key, int *out){
  xmlNodePtr n = (in?xmlGetChildS(in, prop, NULL, NULL):NULL);
  if(!n)return;
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  xmlFreeNode(n);
  if(val) *out = atoi(val);
  xmlFree(val);
}

void xmlGetChildPropIPreserve(xmlNodePtr in, char *prop, char *key, int *out){
  xmlNodePtr n = (in?xmlGetChildSPreserve(in, prop, NULL, NULL):NULL);
  if(!n)return;
  char *val = (char *)xmlGetProp(n, (xmlChar *)key);
  if(val) *out = atoi(val);
  xmlFree(val);
}
  
/* convenience helpers for wielding property maps */
int propmap_pos(propmap **map, int val){
  int i=0;
  propmap *m = *map++;
  while(m){
    if(m->value == val)
      return i;
    i++;
    m = *map++;
  }
  return 0;
}

int propmap_last(propmap **map){
  int i=0;
  propmap *m = *map++;
  while(m){
    i++;
    m = *map++;
  }
  return i-1;
}

int propmap_label_pos(propmap **map, char *label){
  int i=0;
  propmap *m = *map++;
  while(m){
    if(!strcmp(m->left,label))
      return i;
    i++;
    m = *map++;
  }
  return 0;
}
