// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "AdvancedOptionsPage.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ResizableDialogHelper.h"
#include "../Helper/RichEditHelper.h"
#include <glog/logging.h>

const boost::bimap<bool, std::wstring> BOOL_MAPPINGS =
	MakeBimap<bool, std::wstring>({ { true, L"true" }, { false, L"false" } });

AdvancedOptionsPage::AdvancedOptionsPage(HWND parent, HINSTANCE resourceInstance, Config *config,
	CoreInterface *coreInterface, SettingChangedCallback settingChangedCallback,
	HWND tooltipWindow) :
	OptionsPage(IDD_OPTIONS_ADVANCED, IDS_OPTIONS_ADVANCED_TITLE, parent, resourceInstance, config,
		coreInterface, settingChangedCallback, tooltipWindow)
{
}

std::unique_ptr<ResizableDialogHelper> AdvancedOptionsPage::InitializeResizeDialogHelper()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_ADVANCED_OPTIONS), MovingType::None,
		SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_STATIC_ADVANCED_OPTION_DESCRIPTION),
		MovingType::None, SizingType::Horizontal);
	controls.emplace_back(GetDlgItem(GetDialog(), IDC_ADVANCED_OPTION_DESCRIPTION),
		MovingType::None, SizingType::Horizontal);
	return std::make_unique<ResizableDialogHelper>(GetDialog(), controls);
}

void AdvancedOptionsPage::InitializeControls()
{
	HWND listView = GetDlgItem(GetDialog(), IDC_ADVANCED_OPTIONS);

	ListView_SetExtendedListViewStyle(listView,
		LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

	SendDlgItemMessage(GetDialog(), IDC_ADVANCED_OPTION_DESCRIPTION, EM_AUTOURLDETECT,
		AURL_ENABLEURL, NULL);
	SendDlgItemMessage(GetDialog(), IDC_ADVANCED_OPTION_DESCRIPTION, EM_SETEVENTMASK, 0, ENM_LINK);

	std::wstring valueColumnText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_ADVANCED_OPTION_VALUE);

	LV_COLUMN lvColumn;
	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = valueColumnText.data();
	ListView_InsertColumn(listView, 0, &lvColumn);

	std::wstring optionColumnText =
		ResourceHelper::LoadString(m_resourceInstance, IDS_ADVANCED_OPTION);

	lvColumn.mask = LVCF_TEXT;
	lvColumn.pszText = optionColumnText.data();
	ListView_InsertColumn(listView, 1, &lvColumn);

	ListView_SetColumnWidth(listView, 0, LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(listView, 1, LVSCW_AUTOSIZE_USEHEADER);

	int orderArray[] = { 1, 0 };
	ListView_SetColumnOrderArray(listView, SIZEOF_ARRAY(orderArray), orderArray);

	m_advancedOptions = InitializeAdvancedOptions();

	std::sort(m_advancedOptions.begin(), m_advancedOptions.end(),
		[](const AdvancedOption &option1, const AdvancedOption &option2)
		{ return option1.name < option2.name; });

	InsertAdvancedOptionsIntoListView();
}

std::vector<AdvancedOptionsPage::AdvancedOption> AdvancedOptionsPage::InitializeAdvancedOptions()
{
	std::vector<AdvancedOption> advancedOptions;

	AdvancedOption option;
	option.id = AdvancedOptionId::CheckSystemIsPinnedToNameSpaceTree;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_CHECK_PINNED_TO_NAMESPACE_TREE_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_CHECK_PINNED_TO_NAMESPACE_TREE_DESCRIPTION);
	advancedOptions.push_back(option);

	option.id = AdvancedOptionId::OpenTabsInForeground;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_OPEN_TABS_IN_FOREGROUND_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_OPEN_TABS_IN_FOREGROUND_DESCRIPTION);
	advancedOptions.push_back(option);

	option.id = AdvancedOptionId::GoUpOnDoubleClick;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_GO_UP_ON_DOUBLE_CLICK_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = {};
	advancedOptions.push_back(option);

	option.id = AdvancedOptionId::QuickAccessInTreeView;
	option.name = ResourceHelper::LoadString(m_resourceInstance,
		IDS_ADVANCED_OPTION_SHOW_QUICK_ACCESS_IN_TREEVIEW_NAME);
	option.type = AdvancedOptionType::Boolean;
	option.description = {};
	advancedOptions.push_back(option);

	return advancedOptions;
}

void AdvancedOptionsPage::InsertAdvancedOptionsIntoListView()
{
	HWND listView = GetDlgItem(GetDialog(), IDC_ADVANCED_OPTIONS);
	int index = 0;

	for (auto &option : m_advancedOptions)
	{
		LVITEM item;
		item.mask = LVIF_PARAM;
		item.iItem = index++;
		item.iSubItem = 0;
		item.lParam = reinterpret_cast<LPARAM>(&option);
		int finalIndex = ListView_InsertItem(listView, &item);

		if (finalIndex != -1)
		{
			std::wstring value;

			switch (option.type)
			{
			case AdvancedOptionType::Boolean:
				bool booleanValue = GetBooleanConfigValue(option.id);
				value = BOOL_MAPPINGS.left.at(booleanValue);
				break;
			}

			ListView_SetItemText(listView, finalIndex, 0, value.data());
			ListView_SetItemText(listView, finalIndex, 1, option.name.data());
		}
	}
}

bool AdvancedOptionsPage::GetBooleanConfigValue(AdvancedOptionId id)
{
	switch (id)
	{
	case AdvancedOptionId::CheckSystemIsPinnedToNameSpaceTree:
		return m_config->checkPinnedToNamespaceTreeProperty;

	case AdvancedOptionId::OpenTabsInForeground:
		return m_config->openTabsInForeground;

	case AdvancedOptionId::GoUpOnDoubleClick:
		return m_config->goUpOnDoubleClick;

	case AdvancedOptionId::QuickAccessInTreeView:
		return m_config->showQuickAccessInTreeView.get();

	default:
		DCHECK(false);
		break;
	}

	return false;
}

void AdvancedOptionsPage::SetBooleanConfigValue(AdvancedOptionId id, bool value)
{
	switch (id)
	{
	case AdvancedOptionId::CheckSystemIsPinnedToNameSpaceTree:
		m_config->checkPinnedToNamespaceTreeProperty = value;
		break;

	case AdvancedOptionId::OpenTabsInForeground:
		m_config->openTabsInForeground = value;
		break;

	case AdvancedOptionId::GoUpOnDoubleClick:
		m_config->goUpOnDoubleClick = value;
		break;

	case AdvancedOptionId::QuickAccessInTreeView:
		m_config->showQuickAccessInTreeView = value;
		break;

	default:
		DCHECK(false);
		break;
	}
}

INT_PTR AdvancedOptionsPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);

	switch (reinterpret_cast<NMHDR *>(lParam)->code)
	{
	case NM_DBLCLK:
	{
		const auto *info = reinterpret_cast<NMITEMACTIVATE *>(lParam);

		if (info->iItem != -1)
		{
			ListView_EditLabel(info->hdr.hwndFrom, info->iItem);
		}
	}
	break;

	case LVN_ITEMCHANGED:
	{
		auto info = reinterpret_cast<NMLISTVIEW *>(lParam);

		if (WI_IsFlagClear(info->uOldState, LVIS_SELECTED)
			&& WI_IsFlagSet(info->uNewState, LVIS_SELECTED))
		{
			auto *option = reinterpret_cast<AdvancedOption *>(info->lParam);
			SetDlgItemText(GetDialog(), IDC_ADVANCED_OPTION_DESCRIPTION,
				option->description.c_str());
		}
		else if (WI_IsFlagSet(info->uOldState, LVIS_SELECTED)
			&& WI_IsFlagClear(info->uNewState, LVIS_SELECTED))
		{
			// Since only a single item can be selected, if an item has been deselected, it
			// means that there's not currently any item selected (otherwise there would have
			// previously been two items selected).
			SetDlgItemText(GetDialog(), IDC_ADVANCED_OPTION_DESCRIPTION, L"");
		}
	}
	break;

	case LVN_ENDLABELEDIT:
	{
		auto *info = reinterpret_cast<NMLVDISPINFO *>(lParam);

		if (info->item.pszText == nullptr)
		{
			SetWindowLongPtr(GetDialog(), DWLP_MSGRESULT, FALSE);
			return FALSE;
		}

		if (lstrlen(info->item.pszText) == 0)
		{
			SetWindowLongPtr(GetDialog(), DWLP_MSGRESULT, FALSE);
			return FALSE;
		}

		auto *option = GetAdvancedOptionByIndex(info->item.iItem);
		bool validValue = false;

		switch (option->type)
		{
		case AdvancedOptionType::Boolean:
		{
			auto itr = BOOL_MAPPINGS.right.find(info->item.pszText);

			if (itr != BOOL_MAPPINGS.right.end())
			{
				validValue = true;
			}
		}
		break;
		}

		if (!validValue)
		{
			SetWindowLongPtr(GetDialog(), DWLP_MSGRESULT, FALSE);
			return FALSE;
		}

		m_settingChangedCallback();

		SetWindowLongPtr(GetDialog(), DWLP_MSGRESULT, TRUE);
		return TRUE;
	}
	break;

	case EN_LINK:
	{
		const auto *linkNotificationDetails = reinterpret_cast<ENLINK *>(lParam);

		if (linkNotificationDetails->nmhdr.hwndFrom
				== GetDlgItem(GetDialog(), IDC_ADVANCED_OPTION_DESCRIPTION)
			&& linkNotificationDetails->msg == WM_LBUTTONUP)
		{
			std::wstring text = GetRichEditLinkText(linkNotificationDetails);
			ShellExecute(nullptr, L"open", text.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
			return 1;
		}
	}
	break;
	}

	return 0;
}

AdvancedOptionsPage::AdvancedOption *AdvancedOptionsPage::GetAdvancedOptionByIndex(int index)
{
	LVITEM lvItem;
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = index;
	lvItem.iSubItem = 0;
	BOOL res = ListView_GetItem(GetDlgItem(GetDialog(), IDC_ADVANCED_OPTIONS), &lvItem);
	CHECK(res) << "Item lookup failed";

	return reinterpret_cast<AdvancedOption *>(lvItem.lParam);
}

void AdvancedOptionsPage::SaveSettings()
{
	HWND listView = GetDlgItem(GetDialog(), IDC_ADVANCED_OPTIONS);
	int numItems = ListView_GetItemCount(listView);

	for (int i = 0; i < numItems; i++)
	{
		TCHAR text[256];
		ListView_GetItemText(listView, i, 0, text, SIZEOF_ARRAY(text));

		auto &option = m_advancedOptions[i];

		switch (option.type)
		{
		case AdvancedOptionType::Boolean:
		{
			// Values are validated when editing, so the current value should always be valid.
			bool newValue = BOOL_MAPPINGS.right.at(text);
			SetBooleanConfigValue(option.id, newValue);
		}
		break;
		}
	}
}
