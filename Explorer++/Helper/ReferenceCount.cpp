// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ReferenceCount.h"


CReferenceCount::CReferenceCount() :
m_RefCount(1)
{
	
}

ULONG CReferenceCount::AddRef()
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG CReferenceCount::Release()
{
	LONG lCount = InterlockedDecrement(&m_RefCount);

	if(lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}