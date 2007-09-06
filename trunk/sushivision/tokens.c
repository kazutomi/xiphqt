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

// not a true recursive parser as there's no arbitrary nesting.

void _sv_tokenval_free(_sv_tokenval *v){
  if(v){
    if(v->s)free(v->s);
    free(v);
  }
}

void _sv_token_free(_sv_token *t){
  if(t){
    if(t->name)free(t->name);
    if(t->label)free(t->label);
    if(t->values){
      int i;
      for(i=0;i<t->n;i++)
	if(t->values[i])_sv_tokenval_free(t->values[i]);
      free(t->values);
    }
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
  char *head=in;
  char *tail=in;
  if(!in)return NULL;

  while(*head && isspace(*head))head++;
  while(*head){
    *tail = *head;
    tail++;
    head++;
  }
  while(tail>in && isspace(*(tail-1)))tail--;
  *tail=0;

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

char *_sv_tokenize_escape(char *a){
  char *head=a;
  char *tail;
  char *ret;
  int count=0;
  
  while(head && *head){
    if(*head==':' ||
       *head==',' ||
       *head=='(' ||
       *head==')' ||
       isspace(*head) ||
       *head=='\\')
      count++;
    count++;
    head++;
  }

  head=a;
  ret=tail=calloc(count+1,sizeof(*tail));
  
  while(head && *head){
    if(*head==':' ||
       *head==',' ||
       *head=='(' ||
       *head==')' ||
       isspace(*head) ||
       *head=='\\'){
      *tail='\\';
      tail++;
    }
    *tail=*head;
    tail++;
    head++;
  }

  return ret;
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

// a string is any C string of characters not containing unescaped
// parens, commas, colons.  Preceeding and trailing spaces are
// stripped (escaped spaces are never stripped).  Thus, parsing a
// string consists only of stripping spaces and checking for illegal
// characters.
char *_sv_tokenize_string(char *in){
  if(!in)return NULL;
  char *a = strdup(in);
  char *ret = NULL;
  
  // ignore anything following a comma
  if(split(a,','))
    fprintf(stderr,"sushivision: ignoring trailing garbage \"%s\".\n",a);
  
  // ignore anything following a colon
  if(split(a,':'))
    fprintf(stderr,"sushivision: ignoring trailing garbage after \"%s\".\n",a);

  // complain about unescaped parens
  if(unwrap(a))
    fprintf(stderr,"sushivision: ignoring garbage after \"%s\".\n",a);

  if(*a=='\0')goto done;

  ret = strdup(trim(unescape(a)));
  
 done:
  free(a);
  return ret;
}

// a number is a standard printf format floating point number string
// representation.  It may not contain any characters (aside from
// trailing/preceeding spaces) that are not part of the number
// representation.
_sv_tokenval *_sv_tokenize_number(char *in){
  if(!in)return NULL;
  char *a = strdup(in);
  _sv_tokenval *ret=NULL;

  a = trim(unescape(a));
  if(*a=='\0')goto done;

  ret=calloc(1,sizeof(*ret));
  ret->s = strdup(a);
  ret->v = atof_portable(a);

 done:

  free(a);
  return ret;
}

_sv_token *_sv_tokenize_name(char *in){
  _sv_token *ret = NULL;
  char *s = _sv_tokenize_string(in);
  if(!s)return NULL;

  ret=calloc(1,sizeof(*ret));
  ret->name = s;
  return ret;
}

_sv_token *_sv_tokenize_labelname(char *in){
  if(!in)return NULL;
  char *a = strdup(in);
  _sv_token *ret = NULL;

  // split name/label
  char *l=split(a,':');
  ret = _sv_tokenize_name(a);
  if(!ret)goto done;

  if(!l){
    ret->label = strdup(ret->name);
  }else{
    char *label = _sv_tokenize_string(l);
    if(!label)
      ret->label = strdup("");
    else
      ret->label = label;
  }
  
 done:
  free(a);
  return ret;
}

_sv_tokenval *_sv_tokenize_displayvalue(char *in){
  if(!in)return NULL;
  char *a = strdup(in);
  _sv_tokenval *ret = NULL;

  // split value/label
  char *l=split(a,':');
  ret = _sv_tokenize_number(a);
  if(!ret)goto done;

  if(l){
    char *label = _sv_tokenize_string(l);
    if(ret->s) free(ret->s);
    if(!label)
      ret->s = strdup("");
    else
      ret->s = label;
  }
  
 done:
  free(a);
  return ret;
}

_sv_tokenval *_sv_tokenize_flag(char *in){
  _sv_tokenval *ret = NULL;
  char *s = _sv_tokenize_string(in);
  if(!s)return NULL;

  ret=calloc(1,sizeof(*ret));
  ret->s = s;
  ret->v = NAN;
  return ret;
}

_sv_tokenval *_sv_tokenize_parameter(char *in){

  if(!in)return NULL;
  char *a = strdup(in);
  _sv_tokenval *ret = NULL;

  // split value/label
  char *l=split(a,'=');
  if(!l){
    ret = _sv_tokenize_flag(a);
  }else{
    ret = _sv_tokenize_number(l);
    if(ret){
      char *label = _sv_tokenize_string(a);
      if(ret->s) free(ret->s);
      if(!label)
	ret->s = strdup("");
      else
	ret->s = label;
    }
  }
  
  free(a);
  return ret;
}

_sv_token *_sv_tokenize_parameterlist(char *in){
  if(!in)return NULL;

  char *l=strdup(in);
  in=l;

  int i,n = splitcount(l,',');
  _sv_token *ret = calloc(1,sizeof(*ret));

  ret->n = n;
  ret->values = calloc(n,sizeof(*ret->values));

  for(i=0;i<n;i++){
    char *next = split(l,',');
    ret->values[i] = _sv_tokenize_parameter(l);
    l=next;
  }
  free(in);

  return ret;
}

_sv_token *_sv_tokenize_valuelist(char *in){
  if(!in)return NULL;

  char *l=strdup(in);
  in=l;

  int i,n = splitcount(l,',');
  _sv_token *ret = calloc(1,sizeof(*ret));

  ret->n = n;
  ret->values = calloc(n,sizeof(*ret->values));

  for(i=0;i<n;i++){
    char *next = split(l,',');
    ret->values[i] = _sv_tokenize_displayvalue(l);
    l=next;
  }
  free(in);

  return ret;
}

_sv_token *_sv_tokenize_nameparam(char *in){
  _sv_token *ret = NULL;
  char *a=strdup(in);
  char *p;
  if(!a)return NULL;

  // single arg; ignore anything following a level 0 comma
  if(split(a,','))
    fprintf(stderr,"sushivision: ignoring trailing garbage after \"%s\".\n",a);
  
  // split name/args
  p=unwrap(a);

  if(*a=='\0')goto done;
  ret = _sv_tokenize_name(a);

  if(p){
    _sv_token *l = _sv_tokenize_parameterlist(p);
    if(l){
      ret->n = l->n;
      ret->values =  l->values;
      
      l->n = 0;
      l->values = 0;
      _sv_token_free(l);
    }
  }

 done:
  free(a);
  return ret;
}

_sv_token *_sv_tokenize_declparam(char *in){
  _sv_token *ret = NULL;
  char *a=strdup(in);
  char *p;
  if(!a)return NULL;

  // single arg; ignore anything following a level 0 comma
  if(split(a,','))
    fprintf(stderr,"sushivision: ignoring trailing garbage after \"%s\".\n",a);
  
  // split name/args
  p=unwrap(a);

  if(*a=='\0')goto done;
  ret = _sv_tokenize_labelname(a);

  if(p){
    _sv_token *l = _sv_tokenize_parameterlist(p);
    if(l){
      ret->n = l->n;
      ret->values =  l->values;
      
      l->n = 0;
      l->values = 0;
      _sv_token_free(l);
    }
  }

 done:
  free(a);
  return ret;
}

_sv_tokenlist *_sv_tokenize_namelist(char *in){
  if(!in)return NULL;

  char *l=strdup(in);
  in=l;

  int i,n = splitcount(l,',');
  _sv_tokenlist *ret = calloc(1,sizeof(*ret));

  ret->n = n;
  ret->list = calloc(n,sizeof(*ret->list));

  for(i=0;i<n;i++){
    char *next = split(l,',');
    ret->list[i] = _sv_tokenize_nameparam(l);
    l=next;
  }
  free(in);
  
  return ret;
}

