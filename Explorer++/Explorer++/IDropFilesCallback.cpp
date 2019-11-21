// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

/*
 * Handles the case where a file is dropped onto the
 * active listview.
 */

#include "stdafx.h"
#include "IDropFilesCallback.h"
#include "ShellBrowser/ShellBrowser.h"
#include <list>

CDropFilesCallback::CDropFilesCallback(IExplorerplusplus *pexpp) :
m_pexpp(pexpp),
m_RefCount(1)
{

}

HRESULT __stdcall CDropFilesCallback::QueryInterface(REFIID iid,void **ppvObject)
{
	*ppvObject = NULL;

	if(iid == IID_IUnknown)
	{
		*ppvObject = static_cast<IUnknown *>(this);
	}

	if(*ppvObject)
	{
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG __stdcall CDropFilesCallback::AddRef(void)
{
	return ++m_RefCount;
}

ULONG __stdcall CDropFilesCallback::Release(void)
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

void CDropFilesCallback::OnDropFile(const std::list<std::wstring> &PastedFileList, const POINT *ppt)
{
	UNREFERENCED_PARAMETER(ppt);

	if(m_pexpp->GetActiveShellBrowser()->GetNumSelected() == 0)
	{
		m_pexpp->GetActiveShellBrowser()->SelectItems(PastedFileList);
	}
}