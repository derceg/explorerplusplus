// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include "MainResource.h"
#include <map>
#include <unordered_map>

using IconMapping = std::unordered_map<Icon, std::map<int, UINT>>;

#define ICON_SIZE_MAPPINGS(BaseResourceId) \
	{ 16, BaseResourceId##_16 }, \
	{ 24, BaseResourceId##_24 }, \
	{ 32, BaseResourceId##_32 }, \
	{ 48, BaseResourceId##_48 }