// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PidlHelper.h"
#include <list>
#include <vector>

#include <atlbase.h>

class CFileOperationProgressSink : public IFileOperationProgressSink
{
public:
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID iid, LPVOID *ppv)
	{
		HRESULT hr{ E_NOINTERFACE };
		if (!ppv)
			return E_POINTER;

		*ppv = nullptr;
		if (iid == __uuidof(IUnknown))
		{
			*ppv = static_cast<IUnknown *>(this);
			AddRef();
			hr = S_OK;
		}
		else if (iid == __uuidof(IFileOperationProgressSink))
		{
			*ppv = static_cast<IFileOperationProgressSink *>(this);
			AddRef();
			hr = S_OK;
		}

		return hr;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		++m_cRef;
		return m_cRef;
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG cRef = --m_cRef;
		if (0 == cRef)
			delete this;

		return cRef;
	}

	// IFileOperationProgressSink
	STDMETHODIMP StartOperations() { return S_OK; }
	STDMETHODIMP FinishOperations(HRESULT) { return S_OK; }
	STDMETHODIMP PreRenameItem(DWORD, IShellItem *, LPCWSTR) { return S_OK;	}
	STDMETHODIMP PostRenameItem(DWORD, IShellItem *, LPCWSTR, HRESULT, IShellItem *) { return S_OK;	}
	STDMETHODIMP PreMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) { return S_OK;	}
	STDMETHODIMP PostMoveItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) { return S_OK;	}
	STDMETHODIMP PreCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR) { return S_OK;	}
	STDMETHODIMP PostCopyItem(DWORD, IShellItem *, IShellItem *, LPCWSTR, HRESULT, IShellItem *) { return S_OK;	}
	STDMETHODIMP PreDeleteItem(DWORD, IShellItem *) { return S_OK; }
	STDMETHODIMP PreNewItem(DWORD, IShellItem *, LPCWSTR) {	return S_OK; }
	STDMETHODIMP PostNewItem(DWORD, IShellItem *, LPCWSTR, LPCWSTR, DWORD, HRESULT, IShellItem *) {	return S_OK; }
	STDMETHODIMP UpdateProgress(UINT, UINT) { return S_OK; }
	STDMETHODIMP ResetTimer() {	return S_OK; }
	STDMETHODIMP PauseTimer() { return S_OK; }
	STDMETHODIMP ResumeTimer() { return S_OK; }
	STDMETHODIMP PostDeleteItem(DWORD, IShellItem *, HRESULT, IShellItem *psiNewlyCreated)
	{
		HRESULT hr{ S_OK };
		if (psiNewlyCreated)
		{
			PIDLIST_ABSOLUTE pidlNewlyCreated{};
			hr = SHGetIDListFromObject(psiNewlyCreated, &pidlNewlyCreated);
			if (SUCCEEDED(hr) && pidlNewlyCreated)
				m_vPidls.emplace_back(pidlNewlyCreated);
		}
		return hr;
	}

	CFileOperationProgressSink(std::vector<PCIDLIST_ABSOLUTE> &pidls) 
		:m_vPidls(pidls)
	{
	}

private:
	LONG m_cRef{ 1 };
	std::vector<PCIDLIST_ABSOLUTE> &m_vPidls;
	~CFileOperationProgressSink()
	{
	}
};

namespace FileOperations
{

enum class OverwriteMethod
{
	OnePass = 1,
	ThreePass = 2
};

HRESULT RenameFile(IShellItem *item, const std::wstring &newName);
HRESULT DeleteFiles(HWND hwnd, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool permanent, bool silent);
void DeleteFileSecurely(const std::wstring &strFilename, OverwriteMethod overwriteMethod);
HRESULT CopyFilesToFolder(HWND hOwner, const std::wstring &strTitle, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move);
HRESULT CopyFiles(HWND hwnd, IShellItem *destinationFolder, std::vector<PCIDLIST_ABSOLUTE> &pidls, bool move);

HRESULT CreateNewFolder(IShellItem *destinationFolder, const std::wstring &newFolderName, IFileOperationProgressSink *progressSink);

TCHAR *BuildFilenameList(const std::list<std::wstring> &FilenameList);

BOOL SaveDirectoryListing(const std::wstring &strDirectory, const std::wstring &strFilename);

HRESULT CreateLinkToFile(const std::wstring &strTargetFilename, const std::wstring &strLinkFilename,
	const std::wstring &strLinkDescription);
HRESULT ResolveLink(HWND hwnd, DWORD fFlags, const TCHAR *szLinkFilename, TCHAR *szResolvedPath, int nBufferSize);

BOOL CreateBrowseDialog(HWND hOwner, const std::wstring &strTitle, PIDLIST_ABSOLUTE *ppidl);
HRESULT Undelete(const PCIDLIST_ABSOLUTE &pidl);
HRESULT PerformUndeleting(CComPtr<IShellFolder> &shellFolder, const PITEMID_CHILD &pidChild);
HRESULT InvokeVerb(IContextMenu *pContextMenu, PCSTR pszVerb);
};

HRESULT CopyFiles(const std::vector<PidlAbsolute> &items, IDataObject **dataObjectOut);
HRESULT CutFiles(const std::vector<PidlAbsolute> &items, IDataObject **dataObjectOut);
HRESULT CopyFilesToClipboard(const std::vector<PidlAbsolute> &items, bool move, IDataObject **dataObjectOut);
HRESULT CreateFileOperationProgressSink(std::vector<PCIDLIST_ABSOLUTE> &pidls, IFileOperationProgressSink **ppSink);
