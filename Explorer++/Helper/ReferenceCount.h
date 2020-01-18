// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

__interface IReferenceCount
{
	ULONG	AddRef();
	ULONG	Release();
};

class ReferenceCount : public IReferenceCount
{
public:

	ReferenceCount();
	virtual ~ReferenceCount() = default;

	ULONG	AddRef();
	ULONG	Release();

private:

	LONG	m_RefCount;
};