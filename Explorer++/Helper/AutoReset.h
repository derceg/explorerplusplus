// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <utility>

// Performs the same function as AutoReset in the Chromium codebase (i.e. resets a value on
// destruction).
template <typename T>
class [[nodiscard]] AutoReset : private boost::noncopyable
{
public:
	template <typename U>
	AutoReset(T *target, U &&newValue) :
		m_target(target),
		m_originalValue(std::exchange(*m_target, std::forward<U>(newValue)))
	{
	}

	~AutoReset()
	{
		*m_target = std::move(m_originalValue);
	}

private:
	T *m_target = nullptr;
	T m_originalValue;
};
