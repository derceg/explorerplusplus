// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Icon.h"
#include "IconModel.h"

class ResourceLoader;

// Represents an icon that can be loaded from a resource embedded in the application. The advantage
// of this class is that it allows an icon to be specified at a target size, with DPI scaling
// applied once a bitmap is actually requested.
class ResourceIconModel : public IconModel
{
public:
	ResourceIconModel(Icon icon, int size, const ResourceLoader *resourceLoader);

	wil::unique_hbitmap GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const override;

private:
	const Icon m_icon;
	const int m_size;
	const ResourceLoader *const m_resourceLoader;
};
