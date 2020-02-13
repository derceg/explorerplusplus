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
	HRESULT __stdcall	QueryInterface(REFIID iid,void **ppvObject);
	ULONG __stdcall		AddRef(void);
	ULONG __stdcall		Release(void);

private:

	/* IDropFilesCallback methods. */
	void OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt);

	ULONG				m_RefCount;
	IExplorerplusplus	*m_pexpp;
};