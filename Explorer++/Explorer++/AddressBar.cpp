// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AddressBar.h"
#include "AddressBarView.h"
#include "App.h"
#include "AsyncIconFetcher.h"
#include "BrowserWindow.h"
#include "CoreInterface.h"
#include "NavigationHelper.h"
#include "RuntimeHelper.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellBrowser/ShellNavigationController.h"
#include "Tab.h"
#include "TabContainerImpl.h"
#include "../Helper/DragDropHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ShellHelper.h"
#include <glog/logging.h>
#include <wil/com.h>
#include <wil/common.h>

AddressBar *AddressBar::Create(AddressBarView *view, App *app, BrowserWindow *browserWindow,
	CoreInterface *coreInterface)
{
	return new AddressBar(view, app, browserWindow, coreInterface);
}

AddressBar::AddressBar(AddressBarView *view, App *app, BrowserWindow *browserWindow,
	CoreInterface *coreInterface) :
	m_view(view),
	m_app(app),
	m_browserWindow(browserWindow),
	m_coreInterface(coreInterface),
	m_weakPtrFactory(this)
{
	Initialize();
}

void AddressBar::Initialize()
{
	m_view->SetDelegate(this);
	m_view->windowDestroyedSignal.AddObserver(
		std::bind_front(&AddressBar::OnWindowDestroyed, this));

	m_connections.push_back(m_app->GetTabEvents()->AddSelectedObserver(
		std::bind_front(&AddressBar::OnTabSelected, this),
		TabEventScope::ForBrowser(*m_browserWindow)));

	m_connections.push_back(m_app->GetShellBrowserEvents()->AddDirectoryPropertiesChangedObserver(
		std::bind_front(&AddressBar::OnDirectoryPropertiesChanged, this),
		ShellBrowserEventScope::ForBrowser(*m_browserWindow)));

	m_connections.push_back(m_app->GetNavigationEvents()->AddCommittedObserver(
		std::bind_front(&AddressBar::OnNavigationCommitted, this),
		NavigationEventScope::ForBrowser(*m_browserWindow)));
}

AddressBarView *AddressBar::GetView() const
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

	const Tab &selectedTab = m_coreInterface->GetTabContainerImpl()->GetSelectedTab();
	std::wstring currentDirectory = selectedTab.GetShellBrowserImpl()->GetDirectory();

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

	m_browserWindow->OpenItem(*absolutePath,
		DetermineOpenDisposition(false, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT)));
	m_browserWindow->FocusActiveTab();
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
		m_browserWindow->FocusActiveTab();
	}
}

void AddressBar::OnBeginDrag()
{
	const Tab &selectedTab = m_coreInterface->GetTabContainerImpl()->GetSelectedTab();
	auto pidlDirectory = selectedTab.GetShellBrowserImpl()->GetDirectoryIdl();

	SFGAOF attributes = SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK;
	HRESULT hr = GetItemAttributes(pidlDirectory.get(), &attributes);

	if (FAILED(hr)
		|| WI_AreAllFlagsClear(attributes, SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_CANLINK))
	{
		// The root desktop folder is at least one item that can't be copied/moved/linked to. In a
		// situation like that, it's not possible to start a drag at all.
		return;
	}

	wil::com_ptr_nothrow<IDataObject> dataObject;
	std::vector<PCIDLIST_ABSOLUTE> items = { pidlDirectory.get() };
	hr = CreateDataObjectForShellTransfer(items, &dataObject);

	if (FAILED(hr))
	{
		return;
	}

	DWORD allowedEffects = 0;

	if (WI_IsFlagSet(attributes, SFGAO_CANCOPY))
	{
		WI_SetFlag(allowedEffects, DROPEFFECT_COPY);
	}

	if (WI_IsFlagSet(attributes, SFGAO_CANMOVE))
	{
		WI_SetFlag(allowedEffects, DROPEFFECT_MOVE);
	}

	if (WI_IsFlagSet(attributes, SFGAO_CANLINK))
	{
		WI_SetFlag(allowedEffects, DROPEFFECT_LINK);

		hr = SetPreferredDropEffect(dataObject.get(), DROPEFFECT_LINK);
		DCHECK(SUCCEEDED(hr));
	}

	DWORD effect;
	SHDoDragDrop(nullptr, dataObject.get(), nullptr, allowedEffects, &effect);
}

void AddressBar::OnTabSelected(const Tab &tab)
{
	UpdateTextAndIcon(tab);
}

void AddressBar::OnNavigationCommitted(const NavigationRequest *request)
{
	const auto *tab = request->GetShellBrowser()->GetTab();

	if (tab->GetTabContainer()->IsTabSelected(*tab))
	{
		UpdateTextAndIcon(*tab);
	}
}

void AddressBar::OnDirectoryPropertiesChanged(const ShellBrowser *shellBrowser)
{
	const auto *tab = shellBrowser->GetTab();

	if (tab->GetTabContainer()->IsTabSelected(*tab))
	{
		// Since the directory properties have changed, it's possible that the icon has changed.
		// Therefore, the updated icon should always be retrieved.
		UpdateTextAndIcon(*tab, IconUpdateType::AlwaysFetch);
	}
}

void AddressBar::UpdateTextAndIcon(const Tab &tab, IconUpdateType iconUpdateType)
{
	// Resetting this here ensures that any previous icon requests that are still ongoing will be
	// ignored once they complete.
	m_scopedStopSource = std::make_unique<ScopedStopSource>();

	auto entry = tab.GetShellBrowserImpl()->GetNavigationController()->GetCurrentEntry();

	auto cachedIconIndex = m_app->GetIconFetcher()->MaybeGetCachedIconIndex(entry->GetPidl().Raw());
	int iconIndex;

	if (cachedIconIndex)
	{
		iconIndex = *cachedIconIndex;
	}
	else
	{
		iconIndex = m_app->GetIconFetcher()->GetDefaultIconIndex(entry->GetPidl().Raw());
	}

	if (iconUpdateType == IconUpdateType::AlwaysFetch || !cachedIconIndex)
	{
		RetrieveUpdatedIcon(m_weakPtrFactory.GetWeakPtr(), entry->GetPidl(),
			m_app->GetIconFetcher(), m_app->GetRuntime(), m_scopedStopSource->GetToken());
	}

	auto fullPathForDisplay = GetFolderPathForDisplayWithFallback(entry->GetPidl().Raw());
	m_view->UpdateTextAndIcon(fullPathForDisplay, iconIndex);
}

concurrencpp::null_result AddressBar::RetrieveUpdatedIcon(WeakPtr<AddressBar> self,
	PidlAbsolute pidl, std::shared_ptr<AsyncIconFetcher> iconFetcher, Runtime *runtime,
	std::stop_token stopToken)
{
	auto iconInfo = co_await iconFetcher->GetIconIndexAsync(pidl.Raw(), stopToken);

	if (!iconInfo)
	{
		co_return;
	}

	co_await ResumeOnUiThread(runtime);

	if (stopToken.stop_requested() || !self)
	{
		co_return;
	}

	self->m_view->UpdateTextAndIcon(std::nullopt, iconInfo->iconIndex);
}

void AddressBar::OnWindowDestroyed()
{
	delete this;
}
