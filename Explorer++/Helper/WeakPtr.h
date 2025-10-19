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
template <class Data, class View = Data>
	requires(std::same_as<View, Data> || std::same_as<std::remove_const_t<View>, Data>)
class WeakPtr
{
public:
	WeakPtr() : m_state(std::make_shared<WeakState<Data>>())
	{
	}

	View *Get() const
	{
		return m_state->Get();
	}

	View *operator->() const
	{
		return m_state->CheckedGet();
	}

	View &operator*() const
	{
		return *m_state->CheckedGet();
	}

	explicit operator bool() const
	{
		return m_state->IsValid();
	}

	void Reset()
	{
		m_state = std::make_shared<WeakState<Data>>();
	}

private:
	friend class WeakPtrFactory<Data>;

	WeakPtr(std::shared_ptr<const WeakState<Data>> state) : m_state(state)
	{
	}

	std::shared_ptr<const WeakState<Data>> m_state;
};
