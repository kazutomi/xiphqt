/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OGG VORBIS PROJECT SOURCE CODE.         *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OGG VORBIS PROJECT SOURCE CODE IS (C) COPYRIGHT 1994-2001    *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: implementation of the simple queue

 ********************************************************************/

#include <string.h>
#include <stdio.h>

#include "queue.h"

void Queue::Add(void *data)
{
	QueueElement *p;
	QueueElement *e = new QueueElement(data);

	if (m_queue == NULL) {
		m_queue = e;
	} else {
		p = m_queue;
		while (p->m_next != NULL) p = p->m_next;
		p->m_next = e;
	}
}

void *Queue::Remove()
{
	void *data;

	if (m_queue == NULL) return NULL;

	QueueElement *e = m_queue;
	data = e->m_data;
	m_queue = m_queue->m_next;
	delete e;

	return data;
}
