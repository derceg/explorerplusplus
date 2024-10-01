// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Theme.h"
#include "MainResource.h"
#include "ResourceManager.h"
#include <glog/logging.h>

std::wstring GetThemeText(Theme theme)
{
	UINT stringId;

	switch (theme)
	{
	case Theme::Light:
		stringId = IDS_THEME_LIGHT;
		break;

	case Theme::Dark:
		stringId = IDS_THEME_DARK;
		break;

	case Theme::System:
		stringId = IDS_THEME_SYSTEM_DEFAULT;
		break;

	default:
		LOG(FATAL) << "Theme value not found";
		__assume(0);
	}

	return Resources::LoadString(stringId);
}
