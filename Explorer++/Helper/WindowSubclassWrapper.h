// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"
#include <functional>

class WindowSubclassWrapper
{
public:
	using Subclass = std::function<LRESULT(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

	WindowSubclassWrapper(HWND hwnd, SUBCLASSPROC subclassProc, DWORD_PTR data);
	WindowSubclassWrapper(HWND hwnd, Subclass subclass);
	~WindowSubclassWrapper();

private:
	DISALLOW_COPY_AND_ASSIGN(WindowSubclassWrapper);

	static LRESULT CALLBACK SubclassProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);

	HWND m_hwnd;
	SUBCLASSPROC m_subclassProc;
	Subclass m_subclass;
	BOOL m_subclassInstalled;

	// Subclass IDs are managed by this class. When a subclass is installed, it's identified by its
	// subclass procedure and ID. If the same subclass procedure/ID combination is specified, the
	// previous subclass will be replaced.
	// That's a problem in this case, since when using the simpler constructor that accepts a
	// Subclass instance, the subclass procedure will be supplied by this class. If callers could
	// supply their own ID, then two callers could inadvertently overwrite each other (if they were
	// trying to subclass the same window with the same ID).
	// Managing the IDs here ensures that they can by kept unique.
	static inline UINT_PTR m_subclassIdCounter = 0;
	const UINT_PTR m_subclassId;
};
