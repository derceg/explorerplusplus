// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AcceleratorHelper.h"
#include "AcceleratorManager.h"
#include "../Helper/Helper.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <format>

void UpdateMenuItemAcceleratorString(HMENU menu, UINT id,
	const AcceleratorManager *acceleratorManager);
std::optional<std::wstring> VirtualKeyToString(UINT key);

void UpdateMenuAcceleratorStrings(HMENU menu, const AcceleratorManager *acceleratorManager)
{
	int numItems = GetMenuItemCount(menu);

	if (numItems == -1)
	{
		return;
	}

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
			UpdateMenuItemAcceleratorString(menu, id, acceleratorManager);
		}
	}
}

void UpdateMenuItemAcceleratorString(HMENU menu, UINT id,
	const AcceleratorManager *acceleratorManager)
{
	MENUITEMINFO mii;
	TCHAR menuText[256];

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = menuText;
	mii.cch = std::size(menuText);
	BOOL ret = GetMenuItemInfo(menu, id, false, &mii);

	if (!ret)
	{
		return;
	}

	std::optional<ACCEL> accelerator;

	try
	{
		// There is a mismatch here - menu item IDs are UINTs, though the IDs are treated as WORDs
		// in a few different areas. For example, WM_COMMAND treats the IDs as WORDs, as does
		// WM_MENUSELECT. Additionally, the accelerator system also treats the IDs as WORDs, which
		// is the reason for the conversion here.
		accelerator = acceleratorManager->GetAcceleratorForCommand(boost::numeric_cast<WORD>(id));
	}
	catch (const boost::numeric::bad_numeric_cast &)
	{
		// Since menu item IDs are typically treated as words, there shouldn't be any IDs that don't
		// fit within a WORD.
		DCHECK(false);
	}

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

	if (accelerator)
	{
		std::wstring acceleratorString = BuildAcceleratorString(*accelerator);
		text += L"\t" + acceleratorString;
	}

	ZeroMemory(&mii, sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STRING;
	mii.dwTypeData = text.data();
	SetMenuItemInfo(menu, id, false, &mii);
}

std::wstring BuildAcceleratorString(const ACCEL &accelerator)
{
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

	auto keyString = VirtualKeyToString(accelerator.key);

	if (!keyString)
	{
		// It's assumed that mapping the single key above will always succeed, so this branch should
		// never be taken. But if accelerators can be customized by the user, then it seems possible
		// that mapping a particular key in a particular keyboard layout might fail. In which case,
		// having at least something to fall back on is useful.
		DCHECK(false) << std::format("Couldn't convert virtual key {} to string ", accelerator.key);
		keyString = L"?";
	}

	acceleratorParts.push_back(*keyString);

	return boost::algorithm::join(acceleratorParts, L"+");
}

// See https://stackoverflow.com/a/38107083
std::optional<std::wstring> VirtualKeyToString(UINT key)
{
	UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);

	if (scanCode == 0)
	{
		return std::nullopt;
	}

	if (IsExtendedKey(key))
	{
		scanCode |= KF_EXTENDED;
	}

	LONG lParamValue = scanCode << 16;

	TCHAR keyString[32];
	int ret = GetKeyNameText(lParamValue, keyString, std::size(keyString));

	if (ret == 0)
	{
		return std::nullopt;
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
