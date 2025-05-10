// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>

class BrowserWindow;
class ResourceLoader;

class ViewsMenuBuilder
{
public:
	ViewsMenuBuilder(const ResourceLoader *resourceLoader);

	wil::unique_hmenu BuildMenu(const BrowserWindow *browser);
	void AddViewModesToMenu(HMENU menu, UINT startPosition, bool byPosition);

private:
	const ResourceLoader *const m_resourceLoader;
};
