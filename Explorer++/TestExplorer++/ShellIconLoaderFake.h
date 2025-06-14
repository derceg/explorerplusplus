// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellIconLoader.h"
#include <vector>

// A fake ShellIconLoader that generates a new image each time one is requested.
class ShellIconLoaderFake : public ShellIconLoader
{
public:
	using BitmapGeneratedCallback = std::function<void(HBITMAP bitmap)>;

	wil::unique_hbitmap LoadShellIcon(PCIDLIST_ABSOLUTE pidl, ShellIconSize size,
		IconUpdateCallback updateCallback) override;

	void SetBitmapGeneratedCallback(BitmapGeneratedCallback callback);
	void ClearBitmapGeneratedCallback();
	void TriggerPendingUpdateCallbacks();
	void SetStoreUpdateCallbacks(bool storeUpdateCallbacks);

private:
	wil::unique_hbitmap GenerateImage() const;

	std::vector<IconUpdateCallback> m_updateCallbacks;
	bool m_storeUpdateCallbacks = true;
	BitmapGeneratedCallback m_bitmapGeneratedCallback;
};
