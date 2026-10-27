// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarBase.h"
#include "PopupMenuView.h"
#include "ToolbarOverflowMenu.h"
#include <wil/com.h>
#include <vector>

ToolbarBase::ToolbarBase(ToolbarViewBase *view, MenuHelpTextHost *menuHelpTextHost,
	const AcceleratorManager *acceleratorManager) :
	m_acceleratorManager(acceleratorManager),
	m_view(view),
	m_menuHelpTextHost(menuHelpTextHost)
{
}

ToolbarViewBase *ToolbarBase::GetView()
{
	return m_view;
}

bool ToolbarBase::CanShowOverflowMenu() const
{
	return true;
}

void ToolbarBase::ShowOverflowMenu(const POINT &ptScreen)
{
	auto overflowItems = m_view->GetOverflowItems();

	if (overflowItems.empty())
	{
		// Shouldn't be attempting to show the overflow menu when there are no overflow items.
		DCHECK(false);
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
	ToolbarOverflowMenu menu(&popupMenu, m_acceleratorManager, overflowItems, imageList);
	popupMenu.Show(ptScreen);
}
