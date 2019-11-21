// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Macros.h"

class WindowSubclassWrapper
{
public:

	WindowSubclassWrapper(HWND hwnd, SUBCLASSPROC subclassProc, UINT_PTR subclassId, DWORD_PTR data);
	~WindowSubclassWrapper();

	WindowSubclassWrapper(WindowSubclassWrapper &&other);

private:

	DISALLOW_COPY_AND_ASSIGN(WindowSubclassWrapper);

	HWND m_hwnd;
	SUBCLASSPROC m_subclassProc;
	UINT_PTR m_subclassId;
	BOOL m_subclassInstalled;
};