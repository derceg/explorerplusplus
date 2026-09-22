// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "StockIconModel.h"
#include "../Helper/ImageInterop.h"
#include "../Helper/ShellHelper.h"

StockIconModel::StockIconModel(SHSTOCKICONID stockIconId, IconSize size) :
	m_stockIconId(stockIconId),
	m_size(size)
{
}

wil::unique_hbitmap StockIconModel::GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const
{
	// The system will scale the image to the current DPI, but the scaling won't change if the DPI
	// changes. So, this may require more work to handle properly.
	UNREFERENCED_PARAMETER(dpi);

	UNREFERENCED_PARAMETER(updateCallback);

	auto wicIcon = GetStockIconImage(m_stockIconId, m_size);

	if (!wicIcon)
	{
		DCHECK(false);
		return nullptr;
	}

	wil::unique_hbitmap bitmap;
	HRESULT hr = ImageInterop::CreateHBITMAPFromWICBitmap(wicIcon.get(), bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}
