/****************************************************************************
 * 
 *  $Id: fivemque.h,v 1.1 2001/01/31 04:58:50 jack Exp $
 *  
 *  Copyright (C) 1995,1996,1997 RealNetworks, Inc.
 *  All rights reserved.
 *  
 *  http://www.real.com/devzone
 *
 *  This program contains proprietary information of RealNetworks, Inc.,
 *  and is licensed subject to restrictions on use and distribution.
 *
 *  Very basic queue class.
 *
 */
#ifndef _FIVEMQUE_H_
#define _FIVEMQUE_H_


#ifndef _PNTYPES_H_
    #error FiveMinuteQueue assumes pntypes.h.
#endif


/****************************************************************************
 *
 *  FiveMinuteElement Class
 *
 */
class FiveMinuteElement
{
    friend		class FiveMinuteQueue;

    public:
    FiveMinuteElement(void* pPtr)
    {
	m_pPtr = pPtr;
	m_pNext = 0;
    };

    
    private:
    void*		m_pPtr;
    FiveMinuteElement*	m_pNext;

};


/****************************************************************************
 *
 *  FiveMinuteQueue Class
 *
 */
class FiveMinuteQueue
{
    public:
    FiveMinuteQueue()
	: m_pElemList(NULL)
	{};

    void  Add(void* pElement);
    void* Remove();

    
    private:
    FiveMinuteElement*  m_pElemList;

};


#endif /* _FIVEMQUE_H_ */
