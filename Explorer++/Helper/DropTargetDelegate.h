// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <objidl.h>

class DropTargetDelegate
{
public:
	virtual ~DropTargetDelegate() = default;

	virtual DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) = 0;
	virtual DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) = 0;
	virtual void DragLeave() = 0;
	virtual DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) = 0;
};
