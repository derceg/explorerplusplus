// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ReferenceCount.h"

ReferenceCount::ReferenceCount() : m_RefCount(1)
{
}

ULONG ReferenceCount::AddRef()
{
	return InterlockedIncrement(&m_RefCount);
}

ULONG ReferenceCount::Release()
{
	LONG lCount = InterlockedDecrement(&m_RefCount);

	if (lCount == 0)
	{
		delete this;
		return 0;
	}

	return lCount;
}