// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FoldersPaneToolbar.h"
#include "Config.h"
#include "FoldersPaneToolbarView.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/DpiCompatibility.h"

namespace
{

class CloseButton : public ToolbarButton
{
public:
	CloseButton(Config *config, const ResourceLoader *resourceLoader, int imageIndex) :
		ToolbarButton(std::bind(&CloseButton::OnClicked, this)),
		m_config(config),
		m_resourceLoader(resourceLoader),
		m_imageIndex(imageIndex)
	{
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
		return m_resourceLoader->LoadString(IDS_HIDE_FOLDERS_PANE);
	}

	std::optional<int> GetImageIndex() const override
	{
		return m_imageIndex;
	}

private:
	void OnClicked()
	{
		m_config->showFolders = false;
	}

	Config *const m_config;
	const ResourceLoader *const m_resourceLoader;
	const int m_imageIndex;
};

}

FoldersPaneToolbar::FoldersPaneToolbar(FoldersPaneToolbarView *view, Config *config,
	MenuHelpTextHost *menuHelpTextHost, const AcceleratorManager *acceleratorManager,
	const ResourceLoader *resourceLoader) :
	ToolbarBase(view, menuHelpTextHost, acceleratorManager)
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

	auto closeButton = std::make_unique<CloseButton>(config, resourceLoader, closeIconImageIndex);
	view->AddButton(std::move(closeButton), 0);
}
