// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Tab.h"

__interface TabInterface
{
	HRESULT RefreshTab(const Tab &tab);
};