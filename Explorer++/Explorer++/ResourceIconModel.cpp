// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ResourceIconModel.h"
#include "ResourceLoader.h"

ResourceIconModel::ResourceIconModel(Icon icon, int size, const ResourceLoader *resourceLoader) :
	m_icon(icon),
	m_size(std::max(size, 1)),
	m_resourceLoader(resourceLoader)
{
}

wil::unique_hbitmap ResourceIconModel::GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const
{
	UNREFERENCED_PARAMETER(updateCallback);

	return m_resourceLoader->LoadBitmapFromPNGForDpi(m_icon, m_size, m_size, dpi);
}
