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

 function: headers for the  simple queue

 ********************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__

class QueueElement
{
	friend class Queue;

	public:
	QueueElement(void *data) 
	{
		m_data = data;
		m_next = NULL;
	};

	private:
	void *m_data;
	QueueElement *m_next;
};

class Queue
{
	public:
	Queue(): m_queue(NULL)
	{};

	void Add(void *data);
	void *Remove();

	private:
	QueueElement *m_queue;
};

#endif  // __QUEUE_H__
