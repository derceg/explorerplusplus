/******************************************************************
 *
 * Project: Helper
 * File: ReferenceCount.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Basic reference counting class.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "ReferenceCount.h"


CReferenceCount::CReferenceCount()
{
	m_lRefCount = 1;
}

CReferenceCount::~CReferenceCount()
{

}

ULONG CReferenceCount::AddRef()
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG CReferenceCount::Release()
{
	LONG lCount = InterlockedDecrement(&m_lRefCount);

	assert(lCount >= 0);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}