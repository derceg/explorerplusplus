// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/Pidl.h"
#include <boost/outcome.hpp>

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

struct OutcomeResultCheckPolicy : outcome::policy::base
{
	template <class Impl>
	static void wide_value_check(Impl &&self)
	{
		CHECK(base::_has_value(static_cast<Impl &&>(self)));
	}

	template <class Impl>
	static void wide_error_check(Impl &&self)
	{
		CHECK(base::_has_error(static_cast<Impl &&>(self)));
	}
};

class ShellNameParser
{
public:
	using ParseDisplayNameResult = outcome::result<PidlAbsolute, HRESULT, OutcomeResultCheckPolicy>;

	virtual ~ShellNameParser() = default;

	virtual ParseDisplayNameResult ParseDisplayName(const std::wstring &displayName) = 0;
};
