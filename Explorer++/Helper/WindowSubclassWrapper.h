// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <functional>

class WindowSubclassWrapper : private boost::noncopyable
{
public:
	using Subclass = std::function<LRESULT(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)>;

	WindowSubclassWrapper(HWND hwnd, Subclass subclass);
	~WindowSubclassWrapper();

private:
	static LRESULT CALLBACK SubclassProcStub(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR subclassId, DWORD_PTR data);

	const HWND m_hwnd;
	const Subclass m_subclass;
	BOOL m_subclassInstalled;

	// Subclass IDs are managed by this class. When a subclass is installed, it's identified by its
	// subclass procedure and ID. If the same subclass procedure/ID combination is specified, the
	// previous subclass will be replaced.
	// If callers could supply their own ID, then two callers could inadvertently overwrite each
	// other (if they were trying to subclass the same window with the same ID).
	// Managing the IDs here ensures that they can by kept unique.
	static inline UINT_PTR m_subclassIdCounter = 0;
	const UINT_PTR m_subclassId;
};
