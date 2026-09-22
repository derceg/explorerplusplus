// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "IconModel.h"
#include "../Helper/IconSize.h"
#include <shellapi.h>

class StockIconModel : public IconModel
{
public:
	StockIconModel(SHSTOCKICONID stockIconId, IconSize size);

	wil::unique_hbitmap GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const override;

private:
	const SHSTOCKICONID m_stockIconId;
	const IconSize m_size;
};
