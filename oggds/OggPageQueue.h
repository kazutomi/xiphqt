/*******************************************************************************
*                                                                              *
* This file is part of the Ogg Vorbis DirectShow filter collection             *
*                                                                              *
* Copyright (c) 2001, Tobias Waldvogel                                         *
* All rights reserved.                                                         *
*                                                                              *
* Redistribution and use in source and binary forms, with or without           *
* modification, are permitted provided that the following conditions are met:  *
*                                                                              *
*  - Redistributions of source code must retain the above copyright notice,    *
*    this list of conditions and the following disclaimer.                     *
*                                                                              *
*  - Redistributions in binary form must reproduce the above copyright notice, *
*    this list of conditions and the following disclaimer in the documentation *
*    and/or other materials provided with the distribution.                    *
*                                                                              *
*  - The names of the contributors may not be used to endorse or promote       *
*    products derived from this software without specific prior written        *
*    permission.                                                               *
*                                                                              *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"  *
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE    *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE   *
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE     *
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR          *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF         *
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS     *
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN      *
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)      *
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE   *
* POSSIBILITY OF SUCH DAMAGE.                                                  *
*                                                                              *
*******************************************************************************/

#ifndef __OGGPAGEQUEUE__
#define __OGGPAGEQUEUE__

#include <streams.h>
#include <ogg/ogg.h>

typedef struct tPageNode
{
	tPageNode*		pNext;
	REFERENCE_TIME	rtPos;
	ogg_page		og;
	int				header_size;
	int				body_size;
} tPageNode;

class COggPageQueue
{
private:
	CCritSec			m_csPageList;
	tPageNode*			m_pPageListFirst;
	tPageNode*			m_pPageListTail;
	tPageNode*			m_pFreePageList;
	int					m_iPagesInQueue;

	int					m_iHeaderAlloc;
	int					m_iBodyAlloc;

	void				OggPageQueueClearFreeList();

public:
						COggPageQueue();
						~COggPageQueue();
	void				OggPageQueueFlush();
	void				OggPageEnqueue(ogg_page* pog, REFERENCE_TIME* rtTime);
	void				OggPageRelease(tPageNode* pNode);
	tPageNode*			OggPageDequeue();
	int					OggPageQueuePages();
	bool				OggPageQueueCurrTime(REFERENCE_TIME* rtTime);
	bool				OggPageQueuePageReady();
};

#endif