// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomizeColorsDialog.h"
#include "App.h"
#include "ColorRuleEditorDialog.h"
#include "ColorRuleListView.h"
#include "ColorRuleModel.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"
#include "../Helper/WindowSubclass.h"

CustomizeColorsDialog::~CustomizeColorsDialog() = default;

CustomizeColorsDialog::CustomizeColorsDialog(HINSTANCE resourceInstance, HWND parent,
	ThemeManager *themeManager, ColorRuleModel *model,
	const IconResourceLoader *iconResourceLoader) :
	ThemedDialog(resourceInstance, IDD_CUSTOMIZE_COLORS, parent, DialogSizingType::Both,
		themeManager),
	m_model(model),
	m_iconResourceLoader(iconResourceLoader)
{
	m_persistentSettings = &CustomizeColorsDialogPersistentSettings::GetInstance();
}

INT_PTR CustomizeColorsDialog::OnInitDialog()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_LISTVIEW_COLOR_RULES);
	m_colorRuleListView = std::make_unique<ColorRuleListView>(listView, GetResourceInstance(),
		GetThemeManager(), m_model);

	// This object outlives the ColorRuleListView object, so there's no need to remove these
	// observers.
	m_colorRuleListView->colorRuleSelectedSignal.AddObserver(
		std::bind(&CustomizeColorsDialog::UpdateControlStates, this));
	m_colorRuleListView->colorRuleDeselectedSignal.AddObserver(
		std::bind(&CustomizeColorsDialog::UpdateControlStates, this));

	UpdateControlStates();

	SetFocus(listView);

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon CustomizeColorsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_iconResourceLoader->LoadIconFromPNGAndScale(Icon::CustomizeColors, iconWidth,
		iconHeight);
}

std::vector<ResizableDialogControl> CustomizeColorsDialog::GetResizableControls()
{
	std::vector<ResizableDialogControl> controls;
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_LISTVIEW_COLOR_RULES), MovingType::None,
		SizingType::Both);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_NEW), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_EDIT), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_MOVE_UP), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_MOVE_DOWN), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_DELETE), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDC_BUTTON_DELETE_ALL), MovingType::Horizontal,
		SizingType::None);
	controls.emplace_back(GetDlgItem(m_hDlg, IDOK), MovingType::Both, SizingType::None);
	return controls;
}

INT_PTR CustomizeColorsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (LOWORD(wParam))
	{
	case IDC_BUTTON_NEW:
		OnNew();
		break;

	case IDC_BUTTON_EDIT:
		OnEdit();
		break;

	case IDC_BUTTON_MOVE_UP:
		OnMove(MovementDirection::Up);
		break;

	case IDC_BUTTON_MOVE_DOWN:
		OnMove(MovementDirection::Down);
		break;

	case IDC_BUTTON_DELETE:
		OnDelete();
		break;

	case IDC_BUTTON_DELETE_ALL:
		OnDeleteAll();
		break;

	case IDOK:
		OnOk();
		break;
	}

	return 0;
}

INT_PTR CustomizeColorsDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}

void CustomizeColorsDialog::SaveState()
{
	m_persistentSettings->SaveDialogPosition(m_hDlg);

	m_persistentSettings->m_bStateSaved = TRUE;
}

void CustomizeColorsDialog::OnNew()
{
	ColorRuleEditorDialog editorDialog(GetResourceInstance(), m_hDlg, GetThemeManager(), m_model,
		ColorRuleEditorDialog::EditDetails::AddNewColorRule(
			std::make_unique<ColorRule>(L"", L"", false, 0, DEFAULT_INITIAL_COLOR)));
	editorDialog.ShowModalDialog();
}

void CustomizeColorsDialog::OnEdit()
{
	auto *selectedColorRule = m_colorRuleListView->MaybeGetSelectedColorRule();

	if (!selectedColorRule)
	{
		return;
	}

	ColorRuleEditorDialog editorDialog(GetResourceInstance(), m_hDlg, GetThemeManager(), m_model,
		ColorRuleEditorDialog::EditDetails::EditColorRule(selectedColorRule));
	editorDialog.ShowModalDialog();
}

void CustomizeColorsDialog::OnMove(MovementDirection direction)
{
	auto *selectedColorRule = m_colorRuleListView->MaybeGetSelectedColorRule();

	if (!selectedColorRule)
	{
		return;
	}

	auto index = m_model->GetItemIndex(selectedColorRule);
	size_t newIndex;

	if (direction == MovementDirection::Up)
	{
		if (index == 0)
		{
			return;
		}

		newIndex = index - 1;
	}
	else
	{
		if (index == (m_model->GetItems().size() - 1))
		{
			return;
		}

		newIndex = index + 1;
	}

	m_model->MoveItem(selectedColorRule, newIndex);
}

void CustomizeColorsDialog::OnDelete()
{
	auto *selectedColorRule = m_colorRuleListView->MaybeGetSelectedColorRule();

	if (!selectedColorRule)
	{
		return;
	}

	std::wstring deleteMessage =
		ResourceHelper::LoadString(GetResourceInstance(), IDS_COLOR_RULE_DELETE);
	int confirmResult = MessageBox(m_hDlg, deleteMessage.c_str(), App::APP_NAME,
		MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (confirmResult != IDYES)
	{
		return;
	}

	m_model->RemoveItem(selectedColorRule);

	SetFocus(GetDlgItem(m_hDlg, IDC_LISTVIEW_COLOR_RULES));
}

void CustomizeColorsDialog::OnDeleteAll()
{
	std::wstring deleteAllMessage =
		ResourceHelper::LoadString(GetResourceInstance(), IDS_COLOR_RULE_DELETE_ALL);
	int confirmResult = MessageBox(m_hDlg, deleteAllMessage.c_str(), App::APP_NAME,
		MB_YESNO | MB_ICONINFORMATION | MB_DEFBUTTON2);

	if (confirmResult != IDYES)
	{
		return;
	}

	m_model->RemoveAllItems();
}

void CustomizeColorsDialog::OnOk()
{
	EndDialog(m_hDlg, 1);
}

void CustomizeColorsDialog::UpdateControlStates()
{
	auto *selectedColorRule = m_colorRuleListView->MaybeGetSelectedColorRule();
	bool enableItemButtons = (selectedColorRule != nullptr);

	EnableWindow(GetDlgItem(m_hDlg, IDC_BUTTON_EDIT), enableItemButtons);
	EnableWindow(GetDlgItem(m_hDlg, IDC_BUTTON_MOVE_UP), enableItemButtons);
	EnableWindow(GetDlgItem(m_hDlg, IDC_BUTTON_MOVE_DOWN), enableItemButtons);
	EnableWindow(GetDlgItem(m_hDlg, IDC_BUTTON_DELETE), enableItemButtons);
}

CustomizeColorsDialogPersistentSettings::CustomizeColorsDialogPersistentSettings() :
	DialogSettings(SETTINGS_KEY)
{
}

CustomizeColorsDialogPersistentSettings &CustomizeColorsDialogPersistentSettings::GetInstance()
{
	static CustomizeColorsDialogPersistentSettings persistentSettings;
	return persistentSettings;
}
