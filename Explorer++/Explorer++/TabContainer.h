// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class Tab;

class TabContainer
{
public:
	virtual ~TabContainer() = default;

	virtual bool IsTabSelected(const Tab &tab) const = 0;
};
