/****************************************************************************
 * 
 *  $Id: fivemque.cpp,v 1.1 2001/01/31 04:58:50 jack Exp $
 *  
 *  Copyright (C) 1995,1996,1997 RealNetworks.
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


/****************************************************************************
 * Includes
 */
#include <string.h>
#include "pntypes.h"
#include "fivemque.h"


/****************************************************************************
 *  FiveMinuteQueue::Add                                     ref:  fivemque.h
 *
 */
void
FiveMinuteQueue::Add(void* pElement)
{
    FiveMinuteElement*  pElem = new FiveMinuteElement(pElement);

    FiveMinuteElement*  pPtr;
    FiveMinuteElement** ppPtr;

    for (ppPtr = &m_pElemList; (pPtr = *ppPtr) != 0; ppPtr = &pPtr->m_pNext)
	;

    *ppPtr = pElem;
}


/****************************************************************************
 *  FiveMinuteQueue::Remove                                  ref:  fivemque.h
 *
 */
void*
FiveMinuteQueue::Remove()
{
    if (!m_pElemList)
	return 0;

    FiveMinuteElement* pElem = m_pElemList;
    void* pPtr = pElem->m_pPtr;

    m_pElemList = m_pElemList->m_pNext;

    delete pElem;

    return pPtr;
}
