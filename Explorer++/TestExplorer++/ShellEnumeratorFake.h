// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellEnumerator.h"

class ShellEnumeratorFake : public ShellEnumerator
{
public:
	HRESULT EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory, ShellItemFilter::ItemType itemType,
		ShellItemFilter::HiddenItemPolicy hiddenItemPolicy, std::vector<PidlChild> &outputItems,
		std::stop_token stopToken) const override;

	void SetShouldSucceed(bool shouldSucceed);

private:
	bool m_shouldSucceed = true;
};
