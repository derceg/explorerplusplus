// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Config.h"
#include "ThemedDialog.h"
#include "../Helper/DialogSettings.h"

class DisplayColoursDialog;
class DisplayWindow;

class DisplayColoursDialogPersistentSettings : public DialogSettings
{
public:
	static DisplayColoursDialogPersistentSettings &GetInstance();

private:
	friend DisplayColoursDialog;

	static const TCHAR SETTINGS_KEY[];

	DisplayColoursDialogPersistentSettings();

	DisplayColoursDialogPersistentSettings(const DisplayColoursDialogPersistentSettings &);
	DisplayColoursDialogPersistentSettings &operator=(
		const DisplayColoursDialogPersistentSettings &);
};

class DisplayColoursDialog : public ThemedDialog
{
public:
	DisplayColoursDialog(HINSTANCE resourceInstance, HWND hParent, ThemeManager *themeManager,
		Config *config);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnHScroll(HWND hwnd) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	void SaveState() override;

private:
	enum class Color
	{
		Red,
		Green,
		Blue
	};

	struct ColorGroup
	{
		UINT sliderId;
		UINT editId;
		Color color;
	};

	static const int NUM_COLORS = 3;
	static const int TICK_REQUENCY = 10;

	void OnRestoreDefaults();
	void OnChooseFont();
	void OnEnChange(UINT ControlID);

	void OnOk();
	static void CopyDisplayConfigFields(const Config &sourceConfig, Config &destConfig);
	void OnCancel();

	void InitializeColorGroups();
	void InitializeColorGroupControls(ColorGroup colorGroup[NUM_COLORS]);
	void SetColorGroupValues(ColorGroup colorGroup[NUM_COLORS], COLORREF color);
	void InitializePreviewWindow();

	void UpdateEditControlsFromSlider(ColorGroup colorGroup[NUM_COLORS]);
	COLORREF GetColorFromSliderGroup(ColorGroup colorGroup[NUM_COLORS]);

	Config *const m_config;
	Config m_previewConfig;
	DisplayWindow *m_previewDisplayWindow = nullptr;

	ColorGroup m_centerGroup[NUM_COLORS];
	ColorGroup m_surroundingGroup[NUM_COLORS];

	DisplayColoursDialogPersistentSettings *m_pdcdps;
};
