// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <string>

// The advantage of this interface is that it completely separates the task of loading a resource
// from the implementation of it.
class ResourceLoader
{
public:
	virtual ~ResourceLoader() = default;

	virtual std::wstring LoadString(UINT stringId) const = 0;
};
