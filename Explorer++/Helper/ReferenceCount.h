// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

__interface IReferenceCount
{
	ULONG	AddRef();
	ULONG	Release();
};

class CReferenceCount : public IReferenceCount
{
public:

	CReferenceCount();
	virtual ~CReferenceCount() = default;

	ULONG	AddRef();
	ULONG	Release();

private:

	LONG	m_RefCount;
};