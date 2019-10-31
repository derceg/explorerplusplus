// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../ThirdParty/Sol/forward.hpp"
#include <boost/signals2.hpp>
#include <unordered_map>

namespace Plugins
{
	class Event
	{
	public:

		Event();
		virtual ~Event();

		int addObserver(sol::protected_function observer, sol::this_state state);
		void removeObserver(int id);

	protected:

		virtual boost::signals2::connection connectObserver(sol::protected_function observer, sol::this_state state) = 0;

	private:

		int m_connectionIdCounter;
		std::unordered_map<int, boost::signals2::connection> m_connections;
	};
}