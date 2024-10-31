// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <concurrencpp/concurrencpp.h>
#include <unordered_set>

class BrowserWindow;

// Maintains an unordered list of top-level browser windows.
class BrowserList
{
public:
	void AddBrowser(BrowserWindow *browser);
	void RemoveBrowser(BrowserWindow *browser);

	concurrencpp::generator<BrowserWindow *> GetList() const;

private:
	std::unordered_set<BrowserWindow *> m_browsers;
};
