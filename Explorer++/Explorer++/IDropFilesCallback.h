// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DropHandler.h"

class CoreInterface;

class DropFilesCallback : public IDropFilesCallback
{
public:
	DropFilesCallback(CoreInterface *coreInterface);

private:
	/* IDropFilesCallback methods. */
	void OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt) override;

	CoreInterface *m_coreInterface;
};
