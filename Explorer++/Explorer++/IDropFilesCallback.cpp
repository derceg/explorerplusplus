/******************************************************************
 *
 * Project: Explorer++
 * File: IDropFilesCallback.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles the case where a file is dropped onto the
 * active listview.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include <list>
#include "IDropFilesCallback.h"


CDropFilesCallback::CDropFilesCallback(IExplorerplusplus *pexpp) :
m_pexpp(pexpp),
m_RefCount(1)
{

}

CDropFilesCallback::~CDropFilesCallback()
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

void CDropFilesCallback::OnDropFile(const std::list<std::wstring> &PastedFileList,POINT *ppt)
{
	if(m_pexpp->GetActiveShellBrowser()->QueryNumSelected() == 0)
	{
		m_pexpp->GetActiveShellBrowser()->SelectItems(PastedFileList);
	}
}