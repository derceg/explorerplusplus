// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Explorer++_internal.h"
#include "TabInterface.h"
#include "../ThirdParty/Sol/sol.hpp"

namespace Plugins
{
	void BindAllApiMethods(sol::state &state, IExplorerplusplus *pexpp, TabInterface *ti);
}