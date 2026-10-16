// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarOverflowMenu.h"
#include "IconModel.h"
#include "MenuView.h"
#include "ToolbarView.h"
#include "../Helper/Helper.h"
#include "../Helper/ImageHelper.h"
#include <wil/resource.h>

namespace
{

class ToolbarButtonIconModel : public IconModel
{
public:
	ToolbarButtonIconModel(WeakPtr<ToolbarButton> button,
		wil::com_ptr_nothrow<IImageList> imageList);

	// IconModel
	wil::unique_hbitmap GetBitmap(UINT dpi, IconUpdateCallback updateCallback) const override;

private:
	WeakPtr<ToolbarButton> m_button;
	wil::com_ptr_nothrow<IImageList> m_imageList;
};

}

ToolbarOverflowMenu::ToolbarOverflowMenu(MenuView *menuView,
	const AcceleratorManager *acceleratorManager,
	const std::vector<WeakPtr<ToolbarButton>> &overflowButtons,
	const wil::com_ptr_nothrow<IImageList> &imageList) :
	MenuBase(menuView, acceleratorManager)
{
	BuildMenu(overflowButtons, imageList);
}

void ToolbarOverflowMenu::BuildMenu(const std::vector<WeakPtr<ToolbarButton>> &overflowButtons,
	const wil::com_ptr_nothrow<IImageList> &imageList)
{
	for (const auto &button : overflowButtons)
	{
		std::unique_ptr<IconModel> iconModel;

		if (imageList)
		{
			iconModel = std::make_unique<ToolbarButtonIconModel>(button, imageList);
		}

		m_rootMenuView->AppendItem(MenuItemParams(button->GetText())
				.SetIconModel(std::move(iconModel))
				.SetHelpText(button->GetTooltipText())
				.SetSelectedCallback(std::bind_front(&ToolbarOverflowMenu::OnItemSelected, button))
				.SetMiddleClickedCallback(
					std::bind_front(&ToolbarOverflowMenu::OnItemMiddleClicked, button))
				.SetRightClickedCallback(
					std::bind_front(&ToolbarOverflowMenu::OnItemRightClicked, button)));
	}
}

void ToolbarOverflowMenu::OnItemSelected(WeakPtr<ToolbarButton> button, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	if (!button)
	{
		return;
	}

	button->OnClicked({ { 0, 0 }, isShiftKeyDown, isCtrlKeyDown });
}

void ToolbarOverflowMenu::OnItemMiddleClicked(WeakPtr<ToolbarButton> button, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	if (!button)
	{
		return;
	}

	button->OnMiddleClicked({ { 0, 0 }, isShiftKeyDown, isCtrlKeyDown });
}

void ToolbarOverflowMenu::OnItemRightClicked(WeakPtr<ToolbarButton> button, const POINT &ptScreen)
{
	if (!button)
	{
		return;
	}

	button->OnRightClicked({ ptScreen, IsKeyDown(VK_SHIFT), IsKeyDown(VK_CONTROL) });
}

namespace
{

ToolbarButtonIconModel::ToolbarButtonIconModel(WeakPtr<ToolbarButton> button,
	wil::com_ptr_nothrow<IImageList> imageList) :
	m_button(button),
	m_imageList(std::move(imageList))
{
}

wil::unique_hbitmap ToolbarButtonIconModel::GetBitmap(UINT dpi,
	IconUpdateCallback updateCallback) const
{
	UNREFERENCED_PARAMETER(dpi);
	UNREFERENCED_PARAMETER(updateCallback);

	if (!m_button)
	{
		return nullptr;
	}

	auto imageIndex = m_button->GetImageIndex();

	if (!imageIndex)
	{
		return nullptr;
	}

	wil::unique_hbitmap bitmap;
	HRESULT hr =
		ImageHelper::CreateHBITMAPFromImageListIcon(m_imageList.get(), *imageIndex, bitmap);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return bitmap;
}

}
