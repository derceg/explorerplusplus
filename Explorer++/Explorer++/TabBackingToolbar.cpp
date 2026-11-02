// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TabBackingToolbar.h"
#include "BrowserWindow.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "TabBackingToolbarView.h"
#include "TabContainer.h"
#include "TabEvents.h"
#include "../Helper/DpiCompatibility.h"
#include <boost/signals2.hpp>
#include <vector>

namespace
{

class CloseButton : public ToolbarButton
{
public:
	CloseButton(BrowserWindow *browser, TabContainer *tabContainer, TabEvents *tabEvents,
		const ResourceLoader *resourceLoader, int imageIndex) :
		ToolbarButton(std::bind(&CloseButton::OnClicked, this)),
		m_browser(browser),
		m_tabContainer(tabContainer),
		m_resourceLoader(resourceLoader),
		m_imageIndex(imageIndex)
	{
		m_connections.push_back(m_browser->AddLifecycleStateChangedObserver(
			std::bind(&CloseButton::NotifyParentOfUpdate, this)));

		m_connections.push_back(
			tabEvents->AddSelectedObserver(std::bind(&CloseButton::NotifyParentOfUpdate, this),
				TabEventScope::ForBrowser(*m_browser)));
		m_connections.push_back(
			tabEvents->AddUpdatedObserver(std::bind_front(&CloseButton::OnTabUpdated, this),
				TabEventScope::ForBrowser(*m_browser)));
	}

	std::wstring GetText() const override
	{
		return {};
	}

	bool ShouldShowText() const override
	{
		return false;
	}

	std::wstring GetTooltipText() const override
	{
		return m_resourceLoader->LoadString(IDS_TAB_CLOSE_TIP);
	}

	std::optional<int> GetImageIndex() const override
	{
		return m_imageIndex;
	}

	bool IsEnabled() const override
	{
		if (m_browser->GetLifecycleState() != BrowserWindow::LifecycleState::Main)
		{
			return false;
		}

		return m_tabContainer->CanCloseTab(m_tabContainer->GetSelectedTab());
	}

private:
	void OnTabUpdated(const Tab &tab, Tab::PropertyType propertyType)
	{
		switch (propertyType)
		{
		case Tab::PropertyType::LockState:
			if (tab.GetTabContainer()->IsTabSelected(tab))
			{
				NotifyParentOfUpdate();
			}
			break;

		case Tab::PropertyType::Name:
			// Nothing needs to be done in this case.
			break;
		}
	}

	void OnClicked()
	{
		m_tabContainer->CloseTab(m_tabContainer->GetSelectedTab());
	}

	BrowserWindow *const m_browser;
	TabContainer *const m_tabContainer;
	const ResourceLoader *const m_resourceLoader;
	const int m_imageIndex;
	std::vector<boost::signals2::scoped_connection> m_connections;
};

}

TabBackingToolbar::TabBackingToolbar(TabBackingToolbarView *view, BrowserWindow *browser,
	TabContainer *tabContainer, TabEvents *tabEvents, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader) :
	ToolbarBase(view, browser, acceleratorManager)
{
	auto &dpiCompat = DpiCompatibility::GetInstance();
	UINT dpi = dpiCompat.GetDpiForWindow(view->GetHWND());
	int iconWidth = dpiCompat.GetSystemMetricsForDpi(SM_CXSMICON, dpi);
	int iconHeight = dpiCompat.GetSystemMetricsForDpi(SM_CYSMICON, dpi);
	m_imageList.reset(ImageList_Create(iconWidth, iconHeight, ILC_COLOR32 | ILC_MASK, 1, 0));
	CHECK(m_imageList);

	wil::unique_hbitmap closeIcon =
		resourceLoader->LoadBitmapFromPNGAndScale(Icon::CloseButton, iconWidth, iconHeight);
	int closeIconImageIndex = ImageList_Add(m_imageList.get(), closeIcon.get(), nullptr);
	CHECK_NE(closeIconImageIndex, -1);

	view->SetImageList(m_imageList.get());

	auto closeButton = std::make_unique<CloseButton>(browser, tabContainer, tabEvents,
		resourceLoader, closeIconImageIndex);
	view->AddButton(std::move(closeButton), 0);
}
