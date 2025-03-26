// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>

// This class wraps a signal object and is designed to be embedded as a public data member within
// another class. It has the following functionality:
//
// - It adds an AddObserver() method, to allow outside code to connect a slot to the signal.
// - It allows the embedding class to access the internal signal variable.
template <class EmbeddingClassType, typename SignalSignature>
class SignalWrapper
{
	friend EmbeddingClassType;

public:
	using Signal = boost::signals2::signal<SignalSignature>;

	SignalWrapper() = default;

	template <typename... Args>
	boost::signals2::connection AddObserver(Args &&...args)
	{
		return m_signal.connect(std::forward<Args>(args)...);
	}

private:
	SignalWrapper &operator=(const SignalWrapper &) = delete;
	SignalWrapper(const SignalWrapper &) = delete;

	Signal m_signal;
};
