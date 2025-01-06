// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "UniqueThreadId.h"

template <class T>
class WeakState
{
public:
	WeakState(T *ptr = nullptr) :
		m_ptr(ptr),
		m_originalThreadId(UniqueThreadId::GetForCurrentThread())
	{
	}

	void Invalidate()
	{
		DCHECK(m_ptr);
		CHECK(UniqueThreadId::GetForCurrentThread() == m_originalThreadId);

		m_ptr = nullptr;
	}

	bool IsValid() const
	{
		CHECK(UniqueThreadId::GetForCurrentThread() == m_originalThreadId);
		return m_ptr != nullptr;
	}

	T *Get() const
	{
		CHECK(UniqueThreadId::GetForCurrentThread() == m_originalThreadId);
		return m_ptr;
	}

	T *CheckedGet() const
	{
		T *ptr = Get();
		CHECK(ptr);
		return ptr;
	}

private:
	T *m_ptr = nullptr;
	const UniqueThreadId m_originalThreadId;
};
