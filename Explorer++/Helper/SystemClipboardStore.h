// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ClipboardStore.h"

// This class simply forwards each call to the system clipboard.
class SystemClipboardStore : public ClipboardStore
{
public:
	bool Open() override;
	bool Close() override;

	bool IsDataAvailable(UINT format) const override;
	HGLOBAL GetData(UINT format) const override;
	bool SetData(UINT format, wil::unique_hglobal global) override;

	bool Clear() override;
};
