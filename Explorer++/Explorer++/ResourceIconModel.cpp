// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"

ResourceIconModel::ResourceIconModel(Icon icon, IconSize size,
	const ResourceLoader *resourceLoader) :
	m_icon(icon),
	m_size(size),
	m_resourceLoader(resourceLoader)
{
}

wil::unique_hbitmap ResourceIconModel::GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const
{
	UNREFERENCED_PARAMETER(updateCallback);

	int pixelSize = GetIconPixelSizeAtDefaultDpi(m_size);
	return m_resourceLoader->LoadBitmapFromPNGForDpi(m_icon, pixelSize, pixelSize, dpi);
}
