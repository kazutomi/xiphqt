#include <stdlib.h>
#include <string.h>
#include "theora/theora.h"



/*This is more or less the same as strncasecmp, but that doesn't exist
   everywhere, and this is a fairly trivial function, so we include it.
  Note: We take advantage of the fact that we know _n is less than or equal to
   the length of at least one of the strings.*/
static int oc_tagcompare(const char *_s1,const char *_s2,int _n){
  int c;
  for(c=0;c<_n;c++){
    if(toupper(_s1[c])!=toupper(_s2[c]))return !0;
  }
  return 0;
}



void theora_info_init(theora_info *_info){
  memset(_info,0,sizeof(*_info));
  /*c->codec_setup=_ogg_calloc(1,sizeof(codec_setup_info));*/
}

void theora_info_clear(theora_info *_info){
  /*codec_setup_info *ci=c->codec_setup;
  if(ci){
    ClearHuffmanTrees(ci->HuffRoot);
    _ogg_free(ci);
  }*/
  memset(_info,0,sizeof(*_info));
}



void theora_comment_init(theora_comment *_tc){
  memset(_tc,0,sizeof(*_tc));
}

void theora_comment_add(theora_comment *_tc,char *_comment){
  int comment_len;
  _tc->user_comments=_ogg_realloc(_tc->user_comments,
   (_tc->comments+2)*sizeof(*_tc->user_comments));
  _tc->comment_lengths=_ogg_realloc(_tc->comment_lengths,
   (_tc->comments+2)*sizeof(*_tc->comment_lengths));
  comment_len=strlen(_comment);
  _tc->comment_lengths[_tc->comments]=comment_len;
  _tc->user_comments[_tc->comments]=_ogg_malloc(comment_len+1);
  memcpy(_tc->user_comments[_tc->comments],_comment,comment_len+1);
  _tc->comments++;
  _tc->user_comments[_tc->comments]=NULL;
}

void theora_comment_add_tag(theora_comment *_tc,char *_tag,char *_val){
  char *comment;
  int   tag_len;
  int   val_len;
  tag_len=strlen(_tag);
  val_len=strlen(_val);
  /*+2 for '=' and '\0'.*/
  comment=_ogg_malloc(tag_len+val_len+2);
  memcpy(comment,_tag,tag_len);
  comment[tag_len]='=';
  memcpy(comment+tag_len+1,_val,val_len+1);
  theora_comment_add(_tc,comment);
  _ogg_free(comment);
}

char *theora_comment_query(theora_comment *_tc,char *_tag,int _count){
  long  i;
  int   found;
  int   tag_len;
  char *full_tag;
  /*+1 for the '=' we append.*/
  tag_len=strlen(_tag)+1;
  full_tag=_ogg_malloc(tag_len+1);
  memcpy(full_tag,_tag,tag_len-1);
  full_tag[tag_len-1]='=';
  full_tag[tag_len]='\0';
  found=0;
  for(i=0;i<_tc->comments;i++){
    if(!oc_tagcompare(_tc->user_comments[i],full_tag,tag_len)){
      if(_count==found){
        _ogg_free(full_tag);
        /*We return a pointer to the data, not a copy.*/
        return _tc->user_comments[i]+tag_len;
      }
      else found++;
    }
  }
  _ogg_free(full_tag);
  /*Didn't find anything.*/
  return NULL;
}

int theora_comment_query_count(theora_comment *_tc,char *_tag){
  char *full_tag;
  int   tag_len;
  int   count;
  int   i;
  /*+1 for the '=' we append.*/
  tag_len=strlen(_tag)+1;
  full_tag=_ogg_malloc(tag_len+1);
  memcpy(full_tag,_tag,tag_len-1);
  full_tag[tag_len-1]='=';
  full_tag[tag_len]='\0';
  count=0;
  for(i=0;i<_tc->comments;i++){
    if(!oc_tagcompare(_tc->user_comments[i],full_tag,tag_len))count++;
  }
  _ogg_free(full_tag);
  return count;
}

void theora_comment_clear(theora_comment *_tc){
  if(_tc){
    long i;
    for(i=0;i<_tc->comments;i++){
      if(_tc->user_comments[i])_ogg_free(_tc->user_comments[i]);
    }
    if(_tc->user_comments)_ogg_free(_tc->user_comments);
    if(_tc->comment_lengths)_ogg_free(_tc->comment_lengths);
    if(_tc->vendor)_ogg_free(_tc->vendor);
  }
  memset(_tc,0,sizeof(*_tc));
}
