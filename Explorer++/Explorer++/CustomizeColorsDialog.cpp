// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "CustomizeColorsDialog.h"
#include "ColorRuleEditorDialog.h"
#include "ColorRuleListView.h"
#include "ColorRuleModel.h"
#include "CoreInterface.h"
#include "DarkModeHelper.h"
#include "Explorer++_internal.h"
#include "IconResourceLoader.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/Helper.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/WindowHelper.h"

const TCHAR CustomizeColorsDialogPersistentSettings::SETTINGS_KEY[] = _T("CustomizeColors");

static constexpr COLORREF DEFAULT_INITIAL_COLOR = RGB(0, 94, 138);

CustomizeColorsDialog::~CustomizeColorsDialog() = default;

CustomizeColorsDialog::CustomizeColorsDialog(HINSTANCE resourceInstance, HWND parent,
	CoreInterface *coreInterface, ColorRuleModel *model) :
	DarkModeDialogBase(resourceInstance, IDD_CUSTOMIZE_COLORS, parent, true),
	m_coreInterface(coreInterface),
	m_model(model)
{
	m_persistentSettings = &CustomizeColorsDialogPersistentSettings::GetInstance();
}

INT_PTR CustomizeColorsDialog::OnInitDialog()
{
	HWND listView = GetDlgItem(m_hDlg, IDC_LISTVIEW_COLOR_RULES);
	m_colorRuleListView =
		std::make_unique<ColorRuleListView>(listView, GetResourceInstance(), m_model);

	// This object outlives the ColorRuleListView object, so there's no need to remove these
	// observers.
	m_colorRuleListView->colorRuleSelectedSignal.AddObserver(
		std::bind(&CustomizeColorsDialog::UpdateControlStates, this));
	m_colorRuleListView->colorRuleDeselectedSignal.AddObserver(
		std::bind(&CustomizeColorsDialog::UpdateControlStates, this));

	UpdateControlStates();

	SetFocus(listView);

	AllowDarkModeForControls({ IDC_BUTTON_NEW, IDC_BUTTON_EDIT, IDC_BUTTON_MOVE_UP,
		IDC_BUTTON_MOVE_DOWN, IDC_BUTTON_DELETE, IDC_BUTTON_DELETE_ALL });

	m_persistentSettings->RestoreDialogPosition(m_hDlg, true);

	return 0;
}

wil::unique_hicon CustomizeColorsDialog::GetDialogIcon(int iconWidth, int iconHeight) const
{
	return m_coreInterface->GetIconResourceLoader()->LoadIconFromPNGAndScale(Icon::CustomizeColors,
		iconWidth, iconHeight);
}

void CustomizeColorsDialog::GetResizableControlInformation(BaseDialog::DialogSizeConstraint &dsc,
	std::list<ResizableDialog::Control> &ControlList)
{
	dsc = BaseDialog::DialogSizeConstraint::None;

	ResizableDialog::Control control;

	control.iID = IDC_LISTVIEW_COLOR_RULES;
	control.Type = ResizableDialog::ControlType::Resize;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	ControlList.push_back(control);

	control.iID = IDC_BUTTON_NEW;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_BUTTON_EDIT;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_BUTTON_MOVE_UP;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_BUTTON_MOVE_DOWN;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_BUTTON_DELETE;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDC_BUTTON_DELETE_ALL;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::X;
	ControlList.push_back(control);

	control.iID = IDOK;
	control.Type = ResizableDialog::ControlType::Move;
	control.Constraint = ResizableDialog::ControlConstraint::None;
	ControlList.push_back(control);
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
	ColorRuleEditorDialog editorDialog(GetResourceInstance(), m_hDlg, m_model,
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

	ColorRuleEditorDialog editorDialog(GetResourceInstance(), m_hDlg, m_model,
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
	int confirmResult = MessageBox(m_hDlg, deleteMessage.c_str(), NExplorerplusplus::APP_NAME,
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
	int confirmResult = MessageBox(m_hDlg, deleteAllMessage.c_str(), NExplorerplusplus::APP_NAME,
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
