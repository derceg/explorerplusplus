// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PluginMenuManager.h"
#include "TabContainerInterface.h"
#include "UiTheming.h"
#include "../ThirdParty/Sol/sol.hpp"

namespace Plugins
{
	void BindAllApiMethods(sol::state &state, TabContainerInterface *tabContainer,
		PluginMenuManager *pluginMenuManager, UiTheming *uiTheming);
}