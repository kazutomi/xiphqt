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

// This class is used for queueing Ogg pages

#include "OggPageQueue.h"

COggPageQueue::COggPageQueue()
{
	m_pPageListFirst = NULL;
	m_pPageListTail = NULL;
	m_pFreePageList =NULL;
	m_iHeaderAlloc = 0x80;
	m_iBodyAlloc = 0x1400;		
	m_iPagesInQueue = 0;
}

COggPageQueue::~COggPageQueue()
{
	OggPageQueueFlush();
}

void COggPageQueue::OggPageQueueClearFreeList()
{
	// The lock must already be set
	tPageNode*	pTemp;

	while (m_pFreePageList)
	{
		pTemp = m_pFreePageList;
		m_pFreePageList = m_pFreePageList->pNext;
		delete [] pTemp->og.header;
		delete [] pTemp->og.body;
		delete pTemp;
	}
}

// Clears the page list and the free list
void COggPageQueue::OggPageQueueFlush()
{
	CAutoLock	lock(&m_csPageList);

	tPageNode*	pTemp;
	
	// Clear the page list
	while (m_pPageListFirst)
	{
		pTemp = m_pPageListFirst;
		m_pPageListFirst = m_pPageListFirst->pNext;
		delete [] pTemp->og.header;
		delete [] pTemp->og.body;
		delete pTemp;
	}
	m_pPageListTail = NULL;
	m_iPagesInQueue = 0;

	OggPageQueueClearFreeList();
}

// Queues one page
void COggPageQueue::OggPageEnqueue(ogg_page* pog, REFERENCE_TIME* prtTime)
{
	tPageNode*	pNode = NULL;
	CAutoLock	lock(&m_csPageList);

	if ((pog->header_len <= m_iHeaderAlloc) &&
		(pog->body_len   <= m_iBodyAlloc))		// => Page fits
	{
		if (m_pFreePageList)	// Are there pages in the free list ?
		{
			// reuse an existing page
			pNode = m_pFreePageList;
			m_pFreePageList = m_pFreePageList->pNext;
		}
	}
	else
	{
		 // Page doesn't fit => we must increase the size
		if (pog->header_len > m_iHeaderAlloc)
			m_iHeaderAlloc = (pog->header_len & 0xffffff80) + 0x80;
		if (pog->body_len   > m_iBodyAlloc)
			m_iBodyAlloc   = (pog->body_len   & 0xfffffc00) + 0x400;
		OggPageQueueClearFreeList(); // Clear all existing free pages
	}
	
	if (!pNode) // we must create a new one because free list was empty or it didn´t fit
	{
		pNode				= new tPageNode;
		pNode->header_size	= m_iHeaderAlloc;
		pNode->og.header	= new unsigned char[m_iHeaderAlloc];
		pNode->body_size	= m_iBodyAlloc;
		pNode->og.body		= new unsigned char[m_iBodyAlloc];
	}
	
	// Copy the page ...
	memcpy(pNode->og.header, pog->header, pog->header_len);
	pNode->og.header_len = pog->header_len;
	memcpy(pNode->og.body, pog->body, pog->body_len);
	pNode->og.body_len = pog->body_len;
	pNode->rtPos = prtTime ? *prtTime : 0;

	// and add this page at the end of the list
	pNode->pNext = NULL;
	if (m_pPageListTail)
	{
		m_pPageListTail->pNext = pNode;				// Add it as last one
		m_pPageListTail = m_pPageListTail->pNext;	// and let tail point to it
	}
	else
	{
		m_pPageListFirst = pNode;
		m_pPageListTail  = pNode;
	}

	m_iPagesInQueue++;
}

// Get a page from the queue
tPageNode* COggPageQueue::OggPageDequeue()
{
	CAutoLock	lock(&m_csPageList);
	tPageNode*	pPage;

	if (!m_pPageListFirst) return NULL;

	pPage = m_pPageListFirst;
	m_pPageListFirst = m_pPageListFirst->pNext;
	if (m_pPageListFirst == NULL)
		m_pPageListTail = NULL;

	m_iPagesInQueue--;
	return pPage;
}

void COggPageQueue::OggPageRelease(tPageNode* pNode)
{
	if ((pNode->header_size < m_iHeaderAlloc) ||
		(pNode->body_size   < m_iBodyAlloc))
	{
		// This node can not be use anymore because the size has been increased
		delete [] pNode->og.header;
		delete [] pNode->og.body;
		delete pNode;
		return;
	}

	// Otherwise add it to the free list
	CAutoLock	lock(&m_csPageList);

	pNode->pNext = m_pFreePageList;
	m_pFreePageList = pNode;
}

// return the number of pages in the queue
int COggPageQueue::OggPageQueuePages()
{
	CAutoLock	lock(&m_csPageList);
	return m_iPagesInQueue;
}

// checks if the queue is empty
bool COggPageQueue::OggPageQueuePageReady()
{
	CAutoLock	lock(&m_csPageList);
	return m_pPageListFirst != NULL;
}

// returns the time of the first available page
bool COggPageQueue::OggPageQueueCurrTime(REFERENCE_TIME* rtTime)
{
	CAutoLock	lock(&m_csPageList);

	if (!m_pPageListFirst)
		return false;

	*rtTime = m_pPageListFirst->rtPos;
	return true;
}
