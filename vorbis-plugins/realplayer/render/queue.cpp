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

#include "queue.h"

void Queue::Add(void *data)
{
	QueueElement *p;
	QueueElement *e = new QueueElement(data);

	if (m_elements == NULL) {
		m_elements = e;
	} else {
		p = m_elements;
		while (p->m_next != NULL) p = p->m_next;
		p->m_next = e;
	}
}

void *Queue::Remove()
{
	void *data;

	if (m_elements == NULL) return NULL;

	QueueElement *e = m_elements;
	data = e->m_data;
	delete e;

	m_elements = m_elements->m_next;

	return data;
}
