// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/DropHandler.h"

__interface IExplorerplusplus;

class DropFilesCallback : public IDropFilesCallback
{
public:

	DropFilesCallback(IExplorerplusplus *pexpp);

	/* IUnknown methods. */
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject) override;
	ULONG __stdcall		AddRef() override;
	ULONG __stdcall		Release() override;

private:

	/* IDropFilesCallback methods. */
	void OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt) override;

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};