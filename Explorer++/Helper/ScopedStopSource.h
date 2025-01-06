// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <stop_token>

// Wraps a std::stop_source and calls request_stop() on destruction. This class can be useful as a
// class member, for example, since it will call request_stop() when the parent class is destroyed
// and the stop state can then be used to cancel any ongoing asynchronous work.
class ScopedStopSource : private boost::noncopyable
{
public:
	~ScopedStopSource();

	std::stop_token GetToken() const;

private:
	std::stop_source m_stopSource;
};
