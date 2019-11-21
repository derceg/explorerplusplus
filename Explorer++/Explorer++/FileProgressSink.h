// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <functional>

class FileProgressSink : public IFileOperationProgressSink
{
public:

	static FileProgressSink *CreateNew();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	void SetPostNewItemObserver(std::function<void(PIDLIST_ABSOLUTE)> f);

	virtual HRESULT STDMETHODCALLTYPE StartOperations();
	virtual HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT hrResult);
	virtual HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD dwFlags, IShellItem *psiItem, LPCWSTR pszNewName);
	virtual HRESULT STDMETHODCALLTYPE PostRenameItem(DWORD dwFlags, IShellItem *psiItem, LPCWSTR pszNewName, HRESULT hrRename, IShellItem *psiNewlyCreated);
	virtual HRESULT STDMETHODCALLTYPE PreMoveItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName);
	virtual HRESULT STDMETHODCALLTYPE PostMoveItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrMove, IShellItem *psiNewlyCreated);
	virtual HRESULT STDMETHODCALLTYPE PreCopyItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName);
	virtual HRESULT STDMETHODCALLTYPE PostCopyItem(DWORD dwFlags, IShellItem *psiItem, IShellItem *psiDestinationFolder, LPCWSTR pszNewName, HRESULT hrCopy, IShellItem *psiNewlyCreated);
	virtual HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD dwFlags, IShellItem *psiItem);
	virtual HRESULT STDMETHODCALLTYPE PostDeleteItem(DWORD dwFlags, IShellItem *psiItem, HRESULT hrDelete, IShellItem *psiNewlyCreated);
	virtual HRESULT STDMETHODCALLTYPE PreNewItem(DWORD dwFlags, IShellItem *psiDestinationFolder, LPCWSTR pszNewName);
	virtual HRESULT STDMETHODCALLTYPE PostNewItem(DWORD dwFlags, IShellItem *psiDestinationFolder, LPCWSTR pszNewName, LPCWSTR pszTemplateName, DWORD dwFileAttributes, HRESULT hrNew, IShellItem *psiNewItem);
	virtual HRESULT STDMETHODCALLTYPE UpdateProgress(UINT iWorkTotal, UINT iWorkSoFar);
	virtual HRESULT STDMETHODCALLTYPE ResetTimer();
	virtual HRESULT STDMETHODCALLTYPE PauseTimer();
	virtual HRESULT STDMETHODCALLTYPE ResumeTimer();

private:

	FileProgressSink();
	virtual ~FileProgressSink() = default;

	ULONG m_refCount;

	std::function<void(PIDLIST_ABSOLUTE)> m_postNewItemObserver;
};