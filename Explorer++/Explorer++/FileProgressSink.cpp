// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FileProgressSink.h"
#include "../Helper/ShellHelper.h"
#include <boost/scope_exit.hpp>

#pragma warning(disable:4459) // declaration of 'boost_scope_exit_aux_args' hides global declaration

FileProgressSink *FileProgressSink::CreateNew()
{
	return new FileProgressSink();
}

FileProgressSink::FileProgressSink() :
	m_refCount(1)
{

}

HRESULT STDMETHODCALLTYPE FileProgressSink::QueryInterface(REFIID riid, void **ppvObject)
{
	#pragma warning(push)
	#pragma warning(disable:4838) //conversion from 'DWORD' to 'int' requires a narrowing conversion
	static const QITAB qit[] =
	{
		QITABENT(FileProgressSink, IFileOperationProgressSink),
		{nullptr}
	};
	#pragma warning(pop)

	return QISearch(this, qit, riid, ppvObject);
}

ULONG STDMETHODCALLTYPE FileProgressSink::AddRef()
{
	return InterlockedIncrement(&m_refCount);
}

ULONG STDMETHODCALLTYPE FileProgressSink::Release()
{
	ULONG refCount = InterlockedDecrement(&m_refCount);

	if (refCount == 0)
	{
		delete this;
	}

	return refCount;
}

void FileProgressSink::SetPostNewItemObserver(std::function<void(PIDLIST_ABSOLUTE)> f)
{
	m_postNewItemObserver = f;
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

HRESULT STDMETHODCALLTYPE FileProgressSink::PreRenameItem(DWORD dwFlags, IShellItem *psiItem, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostRenameItem(DWORD dwFlags, IShellItem *psiItem, LPCWSTR pszNewName,
	HRESULT hrRename, IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrRename);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreMoveItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostMoveItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder,
	LPCWSTR pszNewName, HRESULT hrMove, IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);
	UNREFERENCED_PARAMETER(hrMove);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreCopyItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostCopyItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder,
	LPCWSTR pszNewName, HRESULT hrCopy, IShellItem *psiNewlyCreated)
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

HRESULT STDMETHODCALLTYPE FileProgressSink::PostDeleteItem(DWORD dwFlags, IShellItem *psiItem, HRESULT hrDelete, IShellItem *psiNewlyCreated)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiItem);
	UNREFERENCED_PARAMETER(hrDelete);
	UNREFERENCED_PARAMETER(psiNewlyCreated);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PreNewItem(DWORD dwFlags, IShellItem *psiDestinationFolder, LPCWSTR pszNewName)
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(psiDestinationFolder);
	UNREFERENCED_PARAMETER(pszNewName);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileProgressSink::PostNewItem(DWORD dwFlags, IShellItem *psiDestinationFolder, LPCWSTR pszNewName,
	LPCWSTR pszTemplateName, DWORD dwFileAttributes, HRESULT hrNew, IShellItem *psiNewItem)
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

	if (FAILED(hrNew))
	{
		return S_OK;
	}

	IUnknown *unknown;
	HRESULT hr = psiNewItem->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&unknown));

	if (FAILED(hr))
	{
		return S_OK;
	}

	BOOST_SCOPE_EXIT(unknown) {
		unknown->Release();
	} BOOST_SCOPE_EXIT_END

	unique_pidl_absolute pidl;
	hr = SHGetIDListFromObject(unknown, wil::out_param(pidl));

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