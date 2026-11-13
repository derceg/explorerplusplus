// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBar.h"
#include "AsyncIconFetcher.h"
#include "BrowserWindow.h"
#include "NavigationHelper.h"
#include "RuntimeHelper.h"
#include "ShellBrowser/NavigationEvents.h"
#include "ShellBrowser/ShellBrowser.h"
#include "ShellBrowser/ShellBrowserEvents.h"
#include "Tab.h"
#include "TabEvents.h"
#include "TestHelper.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>

AddressBar::AddressBar(AddressBarView *view, BrowserWindow *browser, TabEvents *tabEvents,
	ShellBrowserEvents *shellBrowserEvents, NavigationEvents *navigationEvents,
	const Runtime *runtime, AsyncIconFetcher *iconFetcher) :
	m_view(view),
	m_browser(browser),
	m_runtime(runtime),
	m_iconFetcher(iconFetcher),
	m_commandTarget(browser->GetCommandTargetManager(), this),
	m_weakPtrFactory(this)
{
	Initialize(tabEvents, shellBrowserEvents, navigationEvents);
}

void AddressBar::Initialize(TabEvents *tabEvents, ShellBrowserEvents *shellBrowserEvents,
	NavigationEvents *navigationEvents)
{
	m_view->SetDelegate(this);
	m_view->EnableDropTarget(this);

	m_connections.push_back(tabEvents->AddSelectedObserver(
		std::bind_front(&AddressBar::OnTabSelected, this), TabEventScope::ForBrowser(*m_browser)));

	m_connections.push_back(shellBrowserEvents->AddDirectoryPropertiesChangedObserver(
		std::bind_front(&AddressBar::OnDirectoryPropertiesChanged, this),
		ShellBrowserEventScope::ForActiveShellBrowser(*m_browser)));

	m_connections.push_back(navigationEvents->AddCommittedObserver(
		std::bind_front(&AddressBar::OnNavigationCommitted, this),
		NavigationEventScope::ForActiveShellBrowser(*m_browser)));
}

AddressBarView *AddressBar::GetView()
{
	return m_view;
}

bool AddressBar::OnKeyPressed(UINT key)
{
	switch (key)
	{
	case VK_RETURN:
		OnEnterPressed();
		return true;

	case VK_ESCAPE:
		OnEscapePressed();
		return true;
	}

	return false;
}

void AddressBar::OnEnterPressed()
{
	std::wstring path = m_view->GetText();

	const auto *shellBrowser = m_browser->GetActiveShellBrowser();
	std::wstring currentDirectory = shellBrowser->GetDirectoryPath();

	// When entering a path in the address bar in Windows Explorer, environment variables will be
	// expanded. The behavior here is designed to match that.
	// Note that this does result in potential ambiguity. '%' is a valid character in a filename.
	// That means, for example, it's valid to have a file or folder called %windir%. In cases like
	// that, entering the text %windir% would be ambiguous - the path could refer either to the
	// file/folder or environment variable. Explorer treats it as an environment variable, which is
	// also the behavior here.
	// Additionally, it appears that Explorer doesn't normalize "." in paths (though ".." is
	// normalized). For example, entering "c:\windows\.\" results in an error. Whereas here, the
	// path is normalized before navigation, meaning entering "c:\windows\.\" will result in a
	// navigation to "c:\windows". That also means that entering the relative path ".\" works as
	// expected.
	auto absolutePath = TransformUserEnteredPathToAbsolutePathAndNormalize(path, currentDirectory,
		EnvVarsExpansion::Expand);

	if (!absolutePath)
	{
		// TODO: Should possibly display an error here (perhaps in the status bar).
		return;
	}

	/* TODO: Could keep text user has entered and only revert if navigation fails. */
	// Whether a file or folder is being opened, the address bar text should be reverted to the
	// original text. If the item being opened is a folder, the text will be updated once the
	// navigation commits.
	// Note that if the above call to TransformUserEnteredPathToAbsolutePathAndNormalize() fails,
	// the text won't be reverted. That gives the user the chance to update the text and try again.
	m_view->RevertText();

	m_browser->OpenItem(*absolutePath,
		DetermineOpenDisposition(false, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
	m_browser->FocusActiveTab();
}

void AddressBar::OnEscapePressed()
{
	if (m_view->IsTextModified())
	{
		m_view->RevertText();
		m_view->SelectAllText();
	}
	else
	{
		m_browser->FocusActiveTab();
	}
}

void AddressBar::OnBeginDrag()
{
	const auto *shellBrowser = m_browser->GetActiveShellBrowser();
	const auto &pidl = shellBrowser->GetDirectory();
	StartDragForShellItems({ pidl.Raw() }, DROPEFFECT_LINK);
}

void AddressBar::OnFocused()
{
	m_commandTarget.TargetFocused();
}

void AddressBar::OnTabSelected(const Tab &tab)
{
	UpdateTextAndIcon(tab.GetShellBrowser());
}

void AddressBar::OnNavigationCommitted(const NavigationRequest *request)
{
	UpdateTextAndIcon(request->GetShellBrowser());
}

void AddressBar::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	// Since the directory properties have changed, it's possible that the icon has changed.
	// Therefore, the updated icon should always be retrieved.
	UpdateTextAndIcon(shellBrowser, IconUpdateType::AlwaysFetch);
}

void AddressBar::UpdateTextAndIcon(const ShellBrowser *shellBrowser, IconUpdateType iconUpdateType)
{
	// Resetting this here ensures that any previous icon requests that are still ongoing will be
	// ignored once they complete.
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	const auto &pidl = shellBrowser->GetDirectory();

	auto cachedIconIndex = m_iconFetcher->MaybeGetCachedIconIndex(pidl.Raw());
	int iconIndex;

	if (cachedIconIndex)
	{
		iconIndex = *cachedIconIndex;
	}
	else
	{
		iconIndex = m_iconFetcher->GetDefaultIconIndex(pidl.Raw());
	}

	if (iconUpdateType == IconUpdateType::AlwaysFetch || !cachedIconIndex)
	{
		RetrieveUpdatedIcon(m_weakPtrFactory.GetWeakPtr(), pidl);
	}

	auto fullPathForDisplay = GetFolderPathForDisplayWithFallback(pidl.Raw());
	m_view->UpdateTextAndIcon(fullPathForDisplay, iconIndex);
}

concurrencpp::null_result AddressBar::RetrieveUpdatedIcon(WeakPtr<AddressBar> weakSelf,
	PidlAbsolute pidl)
{
	auto *runtime = weakSelf->m_runtime;
	auto iconFetcher = weakSelf->m_iconFetcher;
	auto stopToken = weakSelf->m_scopedStopSource->GetToken();

	auto iconInfo = co_await iconFetcher->GetIconIndexAsync(pidl.Raw(), stopToken);

	if (!iconInfo)
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (stopToken.stop_requested() || !weakSelf)
	{
		co_return;
	}

	weakSelf->m_view->UpdateTextAndIcon(std::nullopt, iconInfo->iconIndex);
}

bool AddressBar::IsCommandEnabled(int command) const
{
	UNREFERENCED_PARAMETER(command);

	return false;
}

void AddressBar::ExecuteCommand(int command)
{
	UNREFERENCED_PARAMETER(command);
}

DWORD AddressBar::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);

	CHECK(std::holds_alternative<std::monostate>(m_droppedData));
	m_droppedData = MaybeExtractDropData(dataObject);
	return GetDropEffect(effect);
}

DWORD AddressBar::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);

	return GetDropEffect(effect);
}

void AddressBar::DragLeave()
{
	m_droppedData = {};
}

DWORD AddressBar::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(pt);

	auto dropEffect = GetDropEffect(effect);
	auto droppedData = std::exchange(m_droppedData, DroppedData{});

	if (dropEffect == DROPEFFECT_NONE)
	{
		return DROPEFFECT_NONE;
	}

	if (const auto *pidl = std::get_if<PidlAbsolute>(&droppedData))
	{
		m_browser->OpenItem(pidl->Raw());
	}
	else if (const auto *text = std::get_if<std::wstring>(&droppedData))
	{
		m_view->SetUserText(*text);
		m_view->Focus();
		m_view->SelectAllText();
	}

	return dropEffect;
}

AddressBar::DroppedData AddressBar::MaybeExtractDropData(IDataObject *dataObject)
{
	PidlAbsolute pidl;
	HRESULT hr = MaybeExtractFirstPidlFromDataObject(dataObject, pidl);

	if (SUCCEEDED(hr))
	{
		return pidl;
	}

	std::wstring text;
	hr = GetTextFromDataObject(dataObject, text);

	if (SUCCEEDED(hr))
	{
		return text;
	}

	return {};
}

HRESULT AddressBar::MaybeExtractFirstPidlFromDataObject(IDataObject *dataObject,
	PidlAbsolute &outputPidl)
{
	wil::com_ptr_nothrow<IShellItemArray> shellItems;
	RETURN_IF_FAILED(SHCreateShellItemArrayFromDataObject(dataObject, IID_PPV_ARGS(&shellItems)));

	wil::com_ptr_nothrow<IShellItem> shellItem;
	RETURN_IF_FAILED(shellItems->GetItemAt(0, &shellItem));

	PidlAbsolute pidl;
	RETURN_IF_FAILED(SHGetIDListFromObject(shellItem.get(), PidlOutParam(pidl)));

	outputPidl = std::move(pidl);

	return S_OK;
}

DWORD AddressBar::GetDropEffect(DWORD allowedEffects) const
{
	if (std::holds_alternative<std::monostate>(m_droppedData))
	{
		return DROPEFFECT_NONE;
	}

	if (WI_IsFlagSet(allowedEffects, DROPEFFECT_COPY))
	{
		return DROPEFFECT_COPY;
	}

	if (WI_IsFlagSet(allowedEffects, DROPEFFECT_LINK))
	{
		return DROPEFFECT_LINK;
	}

	return DROPEFFECT_NONE;
}

DWORD AddressBar::SimulateDropForTest(IDataObject *dataObject, DWORD keyState, POINT pt,
	DWORD effect)
{
	CHECK(IsInTest());

	DragEnter(dataObject, keyState, pt, effect);
	return Drop(dataObject, keyState, pt, effect);
}
