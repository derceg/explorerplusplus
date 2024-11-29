// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialogHelper.h"
#include <memory>

class ColorRuleListView;
class ColorRuleModel;
class CustomizeColorsDialog;
class IconResourceLoader;

class CustomizeColorsDialogPersistentSettings : public DialogSettings
{
public:
	static CustomizeColorsDialogPersistentSettings &GetInstance();

private:
	friend CustomizeColorsDialog;

	static const TCHAR SETTINGS_KEY[];

	CustomizeColorsDialogPersistentSettings();

	CustomizeColorsDialogPersistentSettings(const CustomizeColorsDialogPersistentSettings &);
	CustomizeColorsDialogPersistentSettings &operator=(
		const CustomizeColorsDialogPersistentSettings &);
};

class CustomizeColorsDialog : public ThemedDialog
{
public:
	CustomizeColorsDialog(HINSTANCE resourceInstance, HWND parent, ThemeManager *themeManager,
		ColorRuleModel *model, const IconResourceLoader *iconResourceLoader);
	~CustomizeColorsDialog();

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const override;

private:
	enum class MovementDirection
	{
		Up,
		Down
	};

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
	const IconResourceLoader *const m_iconResourceLoader;
	std::unique_ptr<ColorRuleListView> m_colorRuleListView;

	CustomizeColorsDialogPersistentSettings *m_persistentSettings;
};
