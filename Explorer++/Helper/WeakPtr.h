// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "WeakState.h"
#include <memory>

template <class T>
class WeakPtrFactory;

// This class is designed to be used from a single thread only, since it has no way of guaranteeing
// that the wrapped object will stay alive. In the intended use case, where an object is created and
// deleted on a single thread, checking whether the object is live on that thread is safe.
template <class T>
class WeakPtr
{
public:
	WeakPtr() : m_state(std::make_shared<WeakState<T>>())
	{
	}

	T *Get() const
	{
		return m_state->Get();
	}

	T *operator->() const
	{
		return m_state->CheckedGet();
	}

	T &operator*() const
	{
		return *m_state->CheckedGet();
	}

	explicit operator bool() const
	{
		return m_state->IsValid();
	}

	void Reset()
	{
		m_state = std::make_shared<WeakState<T>>();
	}

private:
	friend class WeakPtrFactory<T>;

	WeakPtr(std::shared_ptr<const WeakState<T>> state) : m_state(state)
	{
	}

	std::shared_ptr<const WeakState<T>> m_state;
};
