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
	const AcceleratorManager *acceleratorManager, const std::vector<ToolbarItem> &overflowItems,
	const wil::com_ptr_nothrow<IImageList> &imageList) :
	MenuBase(menuView, acceleratorManager)
{
	BuildMenu(overflowItems, imageList);
}

void ToolbarOverflowMenu::BuildMenu(const std::vector<ToolbarItem> &overflowItems,
	const wil::com_ptr_nothrow<IImageList> &imageList)
{
	for (const auto &item : overflowItems)
	{
		if (item.IsButton())
		{
			AddButtonToMenu(item.GetButton()->GetWeakPtr(), imageList);
		}
		else
		{
			m_rootMenuView->AppendSeparator();
		}
	}
}

void ToolbarOverflowMenu::AddButtonToMenu(WeakPtr<ToolbarButton> button,
	const wil::com_ptr_nothrow<IImageList> &imageList)
{
	std::unique_ptr<IconModel> iconModel;

	if (imageList)
	{
		iconModel = std::make_unique<ToolbarButtonIconModel>(button, imageList);
	}

	// If the toolbar button is a dropdown, it means it displays a menu. Therefore, the button will
	// be translated into a submenu item.
	if (button->IsDropDownButton())
	{
		auto menuFactory = button->MaybeGetMenuFactory();
		bool hasMenuFactory = !!menuFactory;

		auto *subMenuView = m_rootMenuView->AppendSubMenu(SubMenuItemParams(button->GetText())
				.SetIconModel(std::move(iconModel))
				.SetEnabledCallback([button, hasMenuFactory]
					{ return button && button->IsEnabled() && hasMenuFactory; })
				.SetMiddleClickedCallback(
					std::bind_front(&ToolbarOverflowMenu::OnItemMiddleClicked, button))
				.SetRightClickedCallback(
					std::bind_front(&ToolbarOverflowMenu::OnItemRightClicked, button)));

		if (hasMenuFactory)
		{
			m_childMenus.push_back(menuFactory(subMenuView));
		}
	}
	else
	{
		m_rootMenuView->AppendItem(MenuItemParams(button->GetText())
				.SetIconModel(std::move(iconModel))
				.SetHelpText(button->GetTooltipText())
				.SetEnabledCallback([button] { return button && button->IsEnabled(); })
				.SetCheckedCallback([button] { return button && button->IsChecked(); })
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
