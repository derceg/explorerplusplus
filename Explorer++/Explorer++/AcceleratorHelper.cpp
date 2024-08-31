// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorHelper.h"
#include "AcceleratorManager.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include <boost/algorithm/string/join.hpp>

void UpdateMenuItemAcceleratorString(HMENU menu, UINT id, const std::vector<ACCEL> &accelerators);
std::wstring VirtualKeyToString(UINT key);

void UpdateMenuAcceleratorStrings(HMENU menu, const AcceleratorManager *acceleratorManager)
{
	int numItems = GetMenuItemCount(menu);

	if (numItems == -1)
	{
		return;
	}

	const auto &accelerators = acceleratorManager->GetAccelerators();

	for (int i = 0; i < numItems; i++)
	{
		UINT id = GetMenuItemID(menu, i);

		if (id == -1)
		{
			HMENU subMenu = GetSubMenu(menu, i);

			if (subMenu)
			{
				UpdateMenuAcceleratorStrings(subMenu, acceleratorManager);
			}
		}
		else
		{
			UpdateMenuItemAcceleratorString(menu, id, accelerators);
		}
	}
}

void UpdateMenuItemAcceleratorString(HMENU menu, UINT id, const std::vector<ACCEL> &accelerators)
{
	MENUITEMINFO mii;
	TCHAR menuText[256];

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = menuText;
	mii.cch = SIZEOF_ARRAY(menuText);
	BOOL ret = GetMenuItemInfo(menu, id, false, &mii);

	if (!ret)
	{
		return;
	}

	// Note that there may be multiple key bindings for a particular command. If
	// there are, the block below will simply return the first one.
	auto itr = std::find_if(accelerators.begin(), accelerators.end(),
		[id](const ACCEL &accel) { return accel.cmd == id; });

	std::wstring text = menuText;
	auto tabPosition = text.find('\t');

	// Ideally, the strings for the menu items would be stored independently and
	// the accelerator string would simply be appended, but that's not how
	// things work currently. Therefore, any existing accelerator always needs
	// to be erased first.
	if (tabPosition != std::wstring::npos)
	{
		text.erase(tabPosition);
	}

	if (itr != accelerators.end())
	{
		std::wstring acceleratorString = BuildAcceleratorString(*itr);

		if (!acceleratorString.empty())
		{
			text += L"\t" + acceleratorString;
		}
	}

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = text.data();
	SetMenuItemInfo(menu, id, false, &mii);
}

std::wstring BuildAcceleratorString(const ACCEL &accelerator)
{
	std::wstring keyString = VirtualKeyToString(accelerator.key);

	if (keyString.empty())
	{
		return L"";
	}

	std::vector<std::wstring> acceleratorParts;

	if ((accelerator.fVirt & FCONTROL) == FCONTROL)
	{
		acceleratorParts.emplace_back(L"Ctrl");
	}

	if ((accelerator.fVirt & FALT) == FALT)
	{
		acceleratorParts.emplace_back(L"Alt");
	}

	if ((accelerator.fVirt & FSHIFT) == FSHIFT)
	{
		acceleratorParts.emplace_back(L"Shift");
	}

	acceleratorParts.push_back(keyString);

	return boost::algorithm::join(acceleratorParts, L"+");
}

// See https://stackoverflow.com/a/38107083
std::wstring VirtualKeyToString(UINT key)
{
	UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

	if (IsExtendedKey(key))
	{
		scanCode |= KF_EXTENDED;
	}

	LONG lParamValue = scanCode << 16;

	TCHAR keyString[32];
	int ret = GetKeyNameText(lParamValue, keyString, SIZEOF_ARRAY(keyString));

	if (ret == 0)
	{
		return L"";
	}

	return keyString;
}

std::vector<ACCEL> TableToAcceleratorItems(HACCEL acceleratorTable)
{
	int numAccelerators = CopyAcceleratorTable(acceleratorTable, nullptr, 0);

	std::vector<ACCEL> accelerators(numAccelerators);
	int numAcceleratorsCopied = CopyAcceleratorTable(acceleratorTable, accelerators.data(),
		static_cast<int>(accelerators.size()));
	CHECK_EQ(numAcceleratorsCopied, numAccelerators);

	return accelerators;
}

wil::unique_haccel AcceleratorItemsToTable(std::span<const ACCEL> accelerators)
{
	wil::unique_haccel acceleratorTable(CreateAcceleratorTable(
		const_cast<ACCEL *>(accelerators.data()), static_cast<int>(accelerators.size())));
	CHECK(acceleratorTable);
	return acceleratorTable;
}
