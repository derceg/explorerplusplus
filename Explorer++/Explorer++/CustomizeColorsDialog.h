// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <memory>

class ColorRuleListView;
class ColorRuleModel;
class CustomizeColorsDialog;

class CustomizeColorsDialogPersistentSettings : public DialogSettings
{
public:
	static CustomizeColorsDialogPersistentSettings &GetInstance();

private:
	friend CustomizeColorsDialog;

	static constexpr wchar_t SETTINGS_KEY[] = L"CustomizeColors";

	CustomizeColorsDialogPersistentSettings();

	CustomizeColorsDialogPersistentSettings(const CustomizeColorsDialogPersistentSettings &);
	CustomizeColorsDialogPersistentSettings &operator=(
		const CustomizeColorsDialogPersistentSettings &);
};

class CustomizeColorsDialog : public BaseDialog
{
public:
	static CustomizeColorsDialog *Create(const ResourceLoader *resourceLoader, HWND parent,
		ColorRuleModel *model);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	static constexpr COLORREF DEFAULT_INITIAL_COLOR = RGB(0, 94, 138);

	enum class MovementDirection
	{
		Up,
		Down
	};

	CustomizeColorsDialog(const ResourceLoader *resourceLoader, HWND parent, ColorRuleModel *model);
	~CustomizeColorsDialog();

	std::vector<ResizableDialogControl> GetResizableControls() override;
	void SaveState() override;

	void OnNew();
	void OnEdit();
	void OnMove(MovementDirection direction);
	void OnDelete();
	void OnDeleteAll();

	void OnOk();

	void UpdateControlStates();

	ColorRuleModel *m_model;
	std::unique_ptr<ColorRuleListView> m_colorRuleListView;

	CustomizeColorsDialogPersistentSettings *m_persistentSettings;
};
