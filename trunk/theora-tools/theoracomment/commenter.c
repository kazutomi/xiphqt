/* XXX: Header comment to come */

#include "commenter.h"

#include <stdlib.h>
#include <assert.h>

#define BUFFER_SIZE 4096
#define _(str) str

/* ************************************************************************** */
/* Constructor and destructor */

TheoraCommenter* newTheoraCommenter(TheoraCommenterInput in,
                                    TheoraCommenterOutput out)
{
  TheoraCommenter* ret=malloc(sizeof(TheoraCommenter));
  if(!ret)
    return NULL;

  ret->input=in;
  ret->output=out;

  ret->error=NULL;

  ret->syncState=NULL;
  ret->streamNo=-1;
  ret->stream=NULL;

  theora_comment_init(&ret->comments);
  theora_info_init(&ret->info);

  /* Do this last so an eventual destructor-call will not find us invalid */
  ret->syncState=malloc(sizeof(ogg_stream_state));
  if(!ret->syncState)
  {
    deleteTheoraCommenter(ret);
    return NULL;
  }
  ogg_sync_init(ret->syncState);

  return ret;
}

void deleteTheoraCommenter(TheoraCommenter* me)
{
  if(me->syncState)
    ogg_sync_destroy(me->syncState);
  if(me->stream)
    ogg_stream_destroy(me->stream);
  theora_comment_clear(&me->comments);
  theora_info_clear(&me->info);
  free(me);
}

/* ************************************************************************** */
/* Manipulation */

void theoraCommenter_add(TheoraCommenter* me, char* c)
{
  theora_comment_add(&me->comments, c);
}

void theoraCommenter_addTag(TheoraCommenter* me, char* tag, char* val)
{
  theora_comment_add_tag(&me->comments, tag, val);
}

void theoraCommenter_clear(TheoraCommenter* me)
{
  /* Free everything except vendor-string */
  int i;
  for(i=0; i!=me->comments.comments; ++i)
    free(me->comments.user_comments[i]);
  free(me->comments.user_comments);
  me->comments.user_comments=NULL;
  free(me->comments.comment_lengths);
  me->comments.comment_lengths=NULL;
  me->comments.comments=0;
}

/* ************************************************************************** */
/* Higher-level reading */

int theoraCommenter_read(TheoraCommenter* me)
{
  ogg_packet packet;
  
  /* Find the stream */
  if(theoraCommenter_findTheoraStream(me))
    return 1;

  /* Read the next packet */
  while(!ogg_stream_packetout(me->stream, &packet))
  {
    ogg_page op;
    if(theoraCommenter_readTheoraPage(me, &op))
      return 1;
    if(ogg_stream_pagein(me->stream, &op))
    {
      me->error=_("Error submitting page to stream");
      return 1;
    }
  }

  /* Initialize the comments */
  if(theora_decode_header(&me->info, &me->comments, &packet))
  {
    me->error=_("Error decoding comment header");
    return 1;
  }

  return 0;
}

int theoraCommenter_findTheoraStream(TheoraCommenter* me)
{
  ogg_page op;

  assert(!me->stream);
  assert(me->streamNo==-1);

  /* Find the Theora stream */
  while(1)
  {
    int serialno;
    
    /* Read the next page */
    if(theoraCommenter_readPage(me, &op))
      return 1;
    serialno=ogg_page_serialno(&op);

    /* If not BOS, there's no Theora at all here */
    if(!ogg_page_bos(&op))
    {
      me->error=_("No Theora stream in file");
      return 1;
    }
    
    /* Initialize the stream for this serial */
    if(!me->stream)
    {
      me->stream=malloc(sizeof(ogg_stream_state));
      if(!me->stream)
      {
        me->error=_("Out of memory");
        return 1;
      }
    } else
      ogg_stream_clear(me->stream);
    ogg_stream_init(me->stream, serialno);

    /* Try if this is Theora */
    {
      ogg_packet packet;

      if(ogg_stream_pagein(me->stream, &op))
      {
        me->error=_("Error submitting page to stream");
        return 1;
      }
      if(ogg_stream_packetout(me->stream, &packet)!=1)
      {
        me->error=_("Error getting packet out of stream");
        return 1;
      }

      if(!theora_decode_header(&me->info, &me->comments, &packet))
      {
        /* Found it. */
        me->streamNo=serialno;
        return 0;
      }
    }
  }
  /* Never reached */
}

/* ************************************************************************** */
/* High-level writing */

int theoraCommenter_write(TheoraCommenter* me)
{
  ogg_page op;
  ogg_stream_state outStream; /* This stream is used for output-packetizing */
  int theoraPackets=0; /* How many theora packets have yet been found */

  assert(me->stream);
  assert(me->streamNo!=-1);

  /* Reset syncState as we seeked to the beginning */
  ogg_sync_reset(me->syncState);
  ogg_stream_reset(me->stream);

  ogg_stream_init(&outStream, me->streamNo);
  #define THROW_ERROR \
    { \
      ogg_stream_clear(&outStream); \
      return 1; \
    }

  /* Copy with Theora-packetizing */
  while(1)
  {
    ogg_packet packet;

    /* Read a page in */
    if(theoraCommenter_readPage(me, &op))
      break;

    /* If this is a non-BOS-page and we changed the comments, flush the
     * outStream and continue non-packetizing. */
    if(!ogg_page_bos(&op) && theoraPackets>=2)
    {
      ogg_page pg2;
      while(1)
      {
        if(!ogg_stream_pageout(&outStream, &pg2))
          if(!ogg_stream_flush(&outStream, &pg2))
            break;
        if(theoraCommenter_writePage(me, &pg2))
          THROW_ERROR
      }
      if(theoraCommenter_writePage(me, &op))
        THROW_ERROR
      break;
    }

    /* For Theora, pull it through packetizing */
    /* Replacing the second one with our new comment packet */
    if(ogg_page_serialno(&op)==me->streamNo)
    {
      if(ogg_stream_pagein(me->stream, &op))
      {
        me->error=_("Error submitting page to stream");
        THROW_ERROR
      }

      while(ogg_stream_packetout(me->stream, &packet))
      {
        int isComment;
        ++theoraPackets;
        isComment=(theoraPackets==2);

        /* Replace the packet with our comments */
        if(isComment)
          if(theora_encode_comment(&me->comments, &packet))
          {
            me->error=_("Error encoding comment-header");
            THROW_ERROR
          }

        /* Write the eventually modified packet out */
        if(ogg_stream_packetin(&outStream, &packet))
        {
          me->error=_("Error writing out packet");
          THROW_ERROR
        }

        /* Free the packet when it was encoded */
        if(isComment)
          ogg_packet_clear(&packet);
      }

      if(!ogg_stream_flush(&outStream, &op))
      {
        me->error=_("Unexpected error, there should be packets to flush!");
        THROW_ERROR
      }
    }

    /* Write the page, modified or not */
    if(theoraCommenter_writePage(me, &op))
    {
      me->error=_("Error writing page out");
      THROW_ERROR
    }
  }

  /* Clean up output stream, no longer needed */
  ogg_stream_clear(&outStream);
  #undef THROW_ERROR

  /* Simply copy until the end is reached */
  while(1)
  {
    if(theoraCommenter_readPage(me, &op))
      break;
    if(theoraCommenter_writePage(me, &op))
    {
      me->error=_("Error writing page out");
      return 1;
    }
  }

  return 0;
}

/* ************************************************************************** */
/* Low-level io */

int theoraCommenter_readTheoraPage(TheoraCommenter* me, ogg_page* op)
{
  assert(me->streamNo!=-1);

  /* Find a page which matches the serial-no */ 
  do
  {
    if(theoraCommenter_readPage(me, op))
      return 1;
  } while(ogg_page_serialno(op)!=me->streamNo);

  return 0;
}

int theoraCommenter_readPage(TheoraCommenter* me, ogg_page* op)
{
  while(ogg_sync_pageout(me->syncState, op)!=1)
  {
    char* buf;
    size_t n;

    buf=ogg_sync_buffer(me->syncState, BUFFER_SIZE);
    n=me->input(buf, BUFFER_SIZE);

    /* No input available */
    if(!n)
    {
      me->error=_("Need more input");
      return -1;
    }

    if(ogg_sync_wrote(me->syncState, n))
    {
      me->error=_("Overflowed sync_state-buffer");
      return -1;
    }
  }

  return 0;
}

int theoraCommenter_writePage(TheoraCommenter* me, ogg_page* op)
{
  size_t n;

  /* Is output configured? */
  if(!me->output)
  {
    me->error=_("Can only output when we are in output-mode");
    return 1;
  }

  /* Write header */
  n=me->output(op->header, op->header_len);
  if(n!=op->header_len)
  {
    me->error=_("Error writing output");
    return 1;
  }

  /* Write body */
  n=me->output(op->body, op->body_len);
  if(n!=op->body_len)
  {
    me->error=_("Error writing output");
    return 1;
  }

  return 0;
}
