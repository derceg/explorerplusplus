// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileProgressSink.h"
#include "../Helper/ShellHelper.h"
#include <wil/com.h>

void FileProgressSink::SetPostNewItemObserver(
	std::function<void(PIDLIST_ABSOLUTE)> postNewItemObserver)
{
	m_postNewItemObserver = postNewItemObserver;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::StartOperations()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::FinishOperations(HRESULT hrResult)
{
	UNREFERENCED_PARAMETER(hrResult);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreRenameItem(DWORD dwFlags, IShellItem *psiItem,
	LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostRenameItem(DWORD dwFlags, IShellItem *psiItem,
	LPCWSTR pszNewName, HRESULT hrRename, IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrRename);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreMoveItem(DWORD dwFlags, IShellItem *psiItem,
	IShellItem *psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostMoveItem(DWORD dwFlags, IShellItem *psiItem,
	IShellItem *psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrMove,
	IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrMove);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreCopyItem(DWORD dwFlags, IShellItem *psiItem,
	IShellItem *psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostCopyItem(DWORD dwFlags, IShellItem *psiItem,
	IShellItem *psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrCopy,
	IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrCopy);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreDeleteItem(DWORD dwFlags, IShellItem *psiItem)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostDeleteItem(DWORD dwFlags, IShellItem *psiItem,
	HRESULT hrDelete, IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(hrDelete);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreNewItem(DWORD dwFlags,
	IShellItem *psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostNewItem(DWORD dwFlags,
	IShellItem *psiDestinationFolder, LPCWSTR pszNewName, LPCWSTR pszTemplateName,
	DWORD dwFileAttributes, HRESULT hrNew, IShellItem *psiNewItem)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(pszTemplateName);
	UNREFERENCED_PARAMETER(dwFileAttributes);

	if (!m_postNewItemObserver)
	{
		return S_OK;
	}

	// When attempting to create a folder in a location where the total path length would exceed
	// MAX_PATH, the operation will fail, with hrNew being set to a value of 0x0027000b. That's
	// greater than 0, which means that FAILED(hrNew) will return false. In that case, psiNewItem
	// will be NULL, which will cause a crash if an attempt is made to use it. So, rather than using
	// FAILED, a check is performed against S_OK directly, which is the value used when successful.
	if (hrNew != S_OK)
	{
		return S_OK;
	}

	wil::com_ptr_nothrow<IUnknown> unknown;
	HRESULT hr = psiNewItem->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&unknown));

	if (FAILED(hr))
	{
		return S_OK;
	}

	unique_pidl_absolute pidl;
	hr = SHGetIDListFromObject(unknown.get(), wil::out_param(pidl));

	if (FAILED(hr))
	{
		return S_OK;
	}

	m_postNewItemObserver(pidl.get());

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar)
{
	UNREFERENCED_PARAMETER(iWorkTotal);
	UNREFERENCED_PARAMETER(iWorkSoFar);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::ResetTimer()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PauseTimer()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::ResumeTimer()
{
	return S_OK;
}
