// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

class ShellBrowser;

// The embedder of a ShellBrowser instance can implement this interface to get notified about events
// for the ShellBrowser.
class ShellBrowserEmbedder
{
public:
	virtual ~ShellBrowserEmbedder() = default;

	virtual void OnShellBrowserCreated(ShellBrowser *shellBrowser) = 0;
};
