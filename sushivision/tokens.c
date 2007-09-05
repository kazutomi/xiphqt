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

/* you should never write your own parser.  ever. */

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "internal.h"

void _sv_token_free(_sv_token *t){
  if(t){
    if(t->name)free(t->name);
    if(t->label)free(t->label);
    if(t->options){
      int i;
      for(i=0;i<t->n;i++)
	if(t->options[i])free(t->options[i]);
      free(t->options);
    }
    if(t->values)free(t->values);
    free(t);
  }
}

void _sv_tokenlist_free(_sv_tokenlist *l){
  if(l){
    if(l->list){
      int i;
      for(i=0;i<l->n;i++)
	if(l->list[i])_sv_token_free(l->list[i]);
      free(l->list);
    }
    free(l);
  }
}

// don't allow locale conventions to screw up built-in number strings.
// Sorry, but we're hardcoding decimal points in the float syntax.  This is
// for built-in arg strings, we can substitute in labels later if
// anyone's offended.
static float atof_portable(char *number){
  int msign=1;
  int d=0;
  int f=0;
  int fe=0;
  int esign=1;
  int e=0;
  int dflag=0;
  int fflag=0;
  int eflag=0;

  // trim whitespace
  while(number && *number && isspace(*number))number++;
  
  // overall sign
  if(number && *number=='-'){
    msign = -1;
    number++;
  }
  if(number && *number=='+'){
    number++;
  }

  // whole integer
  while(number && *number>='0' && *number<='9'){
    d = d*10+(*number-48);
    number++;
    dflag=1;
  }

  // seperator
  if(number && *number=='.'){
    number++;
  }
    
  // fraction
  while(number && *number>='0' && *number<='9'){
    f = f*10+(*number-48);
    fe--;
    number++;
    fflag=1;
  }

  // exponent seperator
  if(number && (*number=='e' || *number=='E')){
    number++;

    //exponent sign
    if(number && *number=='-'){
      esign = -1;
      number++;
    }
    if(number && *number=='+'){
      number++;
    }
    while(number && *number>='0' && *number<='9'){
      e = e*10+(*number-48);
      number++;
      eflag=1;
    }
    if(*number)return NAN;
    if(!eflag)return NAN;
  }

  if(!dflag && !fflag)return NAN;
  return msign*(d+f*powf(10,fe))*powf(10,e*esign);
}

static char *trim(char *in){
  char *end;
  if(!in)return NULL;

  while(*in && isspace(*in))in++;
  end=in;
  while(*end)end++;
  while(end>in && (*end==0 || isspace(*end)))end--;
  if(*end)end[1]='\0';

  return in;
}

static char *unescape(char *a){
  char *head=a;
  char *tail=a;
  int escape=0;

  if(head){
    while(1){
      *tail=*head;
      if(!*head)break;
    
      if(*head!='\\' || escape){
	tail++;
	escape=0;
      }else{
	escape=1;
      }
      
      head++;
    }
  }

  return a;
}

// split at unescaped, unenclosed seperator
// only parens enclose in our syntax
static char *split(char *a, char sep){
  char *arg=a;
  char *ret=NULL;
  int escape=0;
  int level=0;

  while(arg && *arg){
    if(*arg=='(' && !escape){
      level++;
    }
    if(*arg==')' && !escape){
      level--;
      if(level<0){
	fprintf(stderr,"sushivision: ignoring extraneous paren in \"%s\".\n",
		a);
	*arg=' ';
	level=0;
      }
    }
    if(*arg==sep && !escape && level==0){
      // we've found our split point
      ret=arg+1;
      *arg='\0';
      return ret;
    }
    if(*arg=='\\'){
      escape=1-escape;
    }else{
      escape=0;
    }
    arg++;
  }

  return NULL;
}

static int splitcount(char *a, char sep){
  char *arg=a;
  int escape=0;
  int level=0;
  int count=1;

  while(arg && *arg){
    if(*arg=='(' && !escape){
      level++;
    }
    if(*arg==')' && !escape){
      level--;
      if(level<0) level=0;
    }
    if(*arg==sep && !escape && level==0)
      count++;
    else{
      if(*arg=='\\'){
	escape=1-escape;
      }else{
	escape=0;
      }
    }
    arg++;
  }

  return count;
}

// unwrap contents enclosed by first level of parens
// only parens enclose in our syntax
static char *unwrap(char *a){
  char *arg=a;
  char *ret=NULL;
  int escape=0;
  int level=0;

  while(arg && *arg){
    if(*arg=='(' && !escape){
      if(level==0){
	ret=arg+1;
	*arg='\0';
      }
      level++;
    }
    if(*arg==')' && !escape){
      level--;
      if(level==0){
	*arg='\0';
	return ret;
      }
      if(level<0){
	fprintf(stderr,"sushivision: ignoring extraneous paren in \"%s\".\n",
		a);
	*arg=' ';
	level=0;
      }
    }
    if(*arg=='\\'){
      escape=1-escape;
    }else{
      escape=0;
    }
    arg++;
  }

  if(level!=0){
    fprintf(stderr,"sushivision: unbalanced paren(s) at \"%s(%s\".\n",
	    a,ret);
    return ret;
  }
  return NULL;
}

// name:label(flag,param=val)
// name:label(val:label, val:label...)
_sv_token *_sv_tokenize(char *in){
  if(!in)return NULL;
  char *a = strdup(in);

  // single arg; ignore anything following a level 0 comma
  if(split(a,','))
    fprintf(stderr,"sushivision: ignoring trailing garbage after \"%s\".\n",a);
  
  // split name/label/args
  char *label=split(a,':');
  if(!label)label=a;
  char *p=unwrap(label);

  if(*a=='\0')goto done;

  _sv_token *ret=calloc(1,sizeof(*ret));
  ret->name = strdup(trim(unescape(a)));
  ret->label = strdup(trim(unescape(label)));

  if(p){
    int i;
    ret->n = splitcount(p,',');
    ret->options = calloc(ret->n,sizeof(*ret->options));
    ret->values = calloc(ret->n,sizeof(*ret->values));
    
    for(i=0;i<ret->n;i++){
      char *next = split(p,',');
      if(p){
	if(*p){
	  // may have param=val or val:label syntax
	  if(splitcount(p,':')>1){
	    
	    if(splitcount(p,'=')>1){
	      fprintf(stderr,"sushivision: parameter \"%s\" contains both \":\" and \"=\"; \"=\" ignored.\n",p);
	    }
	    char *label = split(p,':');
	    ret->options[i]=strdup(trim(unescape(label)));
	    ret->values[i]=atof_portable(trim(unescape(p)));
	    
	  }else if(splitcount(p,'=')>1){
	    
	    char *val = split(p,'=');
	    ret->options[i]=strdup(trim(unescape(p)));
	    ret->values[i]=atof_portable(trim(unescape(val)));
	    
	  }else{
	    ret->options[i]=strdup(trim(unescape(p)));
	    ret->values[i]=atof_portable(trim(unescape(ret->options[i])));
	  }
	}else{
	  ret->values[i]=NAN;
	}
      }
      p=next;
    } 
  }
 done:
  free(a);
  return ret;
}

_sv_tokenlist *_sv_tokenlistize(char *in){
  if(!in)return NULL;

  char *l=strdup(in);
  in=l;

  int i,n = splitcount(l,',');
  _sv_tokenlist *ret = calloc(1,sizeof(*ret));

  ret->n = n;
  ret->list = calloc(n,sizeof(*ret->list));

  for(i=0;i<n;i++){
    char *next = split(l,',');
    ret->list[i] = _sv_tokenize(l);
    l=next;
  }
  free(in);

  return ret;
}

#if 0

int main(int argc, char **argv){
  int i;
  for(i=1;i<argc;i++){
    _sv_tokenlist *ret=_sv_tokenlistize(argv[i]);
    fprintf(stderr,"parsing arglist %d:\n\n",i);
    if(!ret)
      fprintf(stderr,"NULL");
    else{
      int j;
      fprintf(stderr,"arguments: %d",ret->n);
      for(j=0;j<ret->n;j++){
	fprintf(stderr,"\n\tname=%s, label=%s ",
		ret->list[j]->name,
		ret->list[j]->label);
	if(ret->list[j]->n){
	  int k;
	  fprintf(stderr,"(");
	  for(k=0;k<ret->list[j]->n;k++){
	    if(k>0)
	      fprintf(stderr,", ");
	    if(ret->list[j]->options[k]){
	      if(*ret->list[j]->options[k]){
		fprintf(stderr,"%s",ret->list[j]->options[k]);
	      }else{
		fprintf(stderr,"\"\"");
	      }
	    }else{
	      fprintf(stderr,"NULL");
	    }
	    fprintf(stderr,"=%g",ret->list[j]->values[k]);
	  }
	  fprintf(stderr,")");
	}
      }
    }
    fprintf(stderr,"\n\n");
    _sv_tokenlist_free(ret);
  }

  return 0;
}

#endif
