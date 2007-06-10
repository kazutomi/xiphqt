/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggTheora SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

  function: tool for adding comments to Ogg Theora files
  last mod: $Id$

 ********************************************************************/

/**
 * This is the backend-library for theoracomment, defining an object to handle
 * all comment-modifications.
 */

#ifndef THEORACOMMENTER_COMMENTER_H
#define THEORACOMMENTER_COMMENTER_H

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stddef.h>
#include <ogg/ogg.h>
#include <theora/theora.h>

/**
 * An input-callback as used by the commenter.
 * @param buf The buffer to store the data.
 * @param n How many bytes to read.
 * @return Number bytes read.
 */
typedef size_t (*TheoraCommenterInput)(char* buf, size_t n);

/**
 * An output-callback used by the commenter.
 * @param buf The data to write.
 * @param n How many bytes to write.
 * @return Number bytes written.
 */
typedef size_t (*TheoraCommenterOutput)(char* buf, size_t n);

/**
 * An object controlling comment-editing for theora-streams.
 */
typedef struct
{
  /** The input used */
  TheoraCommenterInput input;
  /** The output used */
  TheoraCommenterOutput output;

  /** The libogg-sync-state */
  ogg_sync_state* syncState;

  /** The serialno of the theora-stream if yet found */
  int streamNo;
  /** The theora-stream itself */
  ogg_stream_state* stream;

  /** The comments found */
  theora_comment comments;
  /** The info header */
  theora_info info;

  /** The last error occured or NULL if none */
  const char* error;
} TheoraCommenter;

/**
 * Create a new TheoraCommenter.
 * @param in The input to use.
 * @param out The output to use (may be null if no output is needed)
 * @return The created TheoraCommenter.
 */
TheoraCommenter* newTheoraCommenter(TheoraCommenterInput in,
                                    TheoraCommenterOutput out);

/**
 * Destroy and free a TheoraCommenter.
 * @param me The commenter to free.
 */
void deleteTheoraCommenter(TheoraCommenter* me);

/**
 * Read in the input and store the found comments.
 * @param me The this-pointer
 * @return 0 if it succeeded.
 */
int theoraCommenter_read(TheoraCommenter* me);

/**
 * Write the whole thing out with possibly modified comments; before doing so,
 * ensure that the input-stream has been seeked back to the very beginning!
 * @param me this
 * @return 0 on success
 */
int theoraCommenter_write(TheoraCommenter* me);

/**
 * Clear the comment-structure, i.e., remove all comments but preserve the
 * vendor-string.
 */
void theoraCommenter_clear(TheoraCommenter* me);

/**
 * Append a comment to the structure.  Always succeeds.
 * @param me this
 * @param c The comment as "TAG=value"
 */
void theoraCommenter_add(TheoraCommenter* me, char* c);
/**
 * Append a comment as tag and value, always succeeds.
 * @param me this
 * @param tag The comment's tag
 * @param val The comment's value
 */
void theoraCommenter_addTag(TheoraCommenter* me, char* tag, char* val);

/**
 * Find the Theora stream.
 * @param me this
 * @return 0 if success
 */
int theoraCommenter_findTheoraStream(TheoraCommenter* me);

/**
 * Read the next Theora-page in.
 * @param me this
 * @param op The page to be filld.
 * @return 0 on success.
 */
int theoraCommenter_readTheoraPage(TheoraCommenter* me, ogg_page* op);

/**
 * Read a page from the input.
 * @param me this
 * @param op The page to be filled.
 * @return 0 if success.
 */
int theoraCommenter_readPage(TheoraCommenter* me, ogg_page* op);

/**
 * Write a page to the output.
 * @param me this
 * @param op The page to be output.
 * @return 0 on success
 */
int theoraCommenter_writePage(TheoraCommenter* me, ogg_page* op);

#endif /* Headerguard */
