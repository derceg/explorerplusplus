// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarBase.h"
#include "PopupMenuView.h"
#include "ToolbarOverflowMenu.h"
#include "ToolbarView.h"
#include <wil/com.h>
#include <vector>

ToolbarBase::ToolbarBase(ToolbarView *view, MenuHelpTextHost *menuHelpTextHost,
	const AcceleratorManager *acceleratorManager) :
	m_view(view),
	m_acceleratorManager(acceleratorManager),
	m_menuHelpTextHost(menuHelpTextHost)
{
}

ToolbarView *ToolbarBase::GetView() const
{
	return m_view;
}

void ToolbarBase::ShowOverflowMenu(const POINT &ptScreen)
{
	std::vector<WeakPtr<ToolbarButton>> overflowButtons;
	auto numButtons = m_view->GetButtons().size();

	for (size_t i = 0; i < numButtons; i++)
	{
		if (m_view->IsButtonVisible(i))
		{
			continue;
		}

		auto *button = m_view->GetButton(i);
		overflowButtons.push_back(button->GetWeakPtr());
	}

	if (overflowButtons.empty())
	{
		return;
	}

	auto imageListHandle = m_view->MaybeGetImageList();
	wil::com_ptr_nothrow<IImageList> imageList;

	if (imageListHandle)
	{
		auto hr = HIMAGELIST_QueryInterface(imageListHandle, IID_PPV_ARGS(&imageList));
		CHECK(SUCCEEDED(hr));
	}

	PopupMenuView popupMenu(m_view->GetHWND(), m_menuHelpTextHost);
	ToolbarOverflowMenu menu(&popupMenu, m_acceleratorManager, overflowButtons, imageList);
	popupMenu.Show(ptScreen);
}
