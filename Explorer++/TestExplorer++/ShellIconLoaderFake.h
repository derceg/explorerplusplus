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
	wil::com_ptr_nothrow<IWICBitmapSource> LoadShellIcon(PCIDLIST_ABSOLUTE pidl, IconSize size,
		IconUpdateCallback updateCallback, std::stop_token stopToken) override;

	int GetNumIconsRequested() const;
	void TriggerPendingUpdateCallbacks();
	void SetStoreUpdateCallbacks(bool storeUpdateCallbacks);

private:
	struct PendingUpdate
	{
		IconUpdateCallback callback;
		std::stop_token stopToken;
	};

	wil::com_ptr_nothrow<IWICBitmapSource> GenerateImage() const;

	std::vector<PendingUpdate> m_pendingUpdates;
	bool m_storeUpdateCallbacks = true;
	int m_numIconsRequested = 0;
};
