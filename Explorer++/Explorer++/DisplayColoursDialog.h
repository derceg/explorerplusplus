// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DarkModeDialogBase.h"
#include "../Helper/DialogSettings.h"

class DisplayColoursDialog;

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

class DisplayColoursDialog : public DarkModeDialogBase
{
public:
	DisplayColoursDialog(HINSTANCE hInstance, HWND hParent, HWND hDisplayWindow,
		COLORREF DefaultCenterColor, COLORREF DefaultSurroundingColor);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnHScroll(HWND hwnd) override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;
	INT_PTR OnDestroy() override;

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
	void OnCancel();

	void InitializeColorGroups();
	void InitializeColorGroupControls(ColorGroup colorGroup[NUM_COLORS]);
	void SetColorGroupValues(ColorGroup colorGroup[NUM_COLORS], COLORREF color);
	void InitializePreviewWindow();

	void UpdateEditControlsFromSlider(ColorGroup colorGroup[NUM_COLORS]);
	COLORREF GetColorFromSliderGroup(ColorGroup colorGroup[NUM_COLORS]);

	HWND m_hDisplayWindow;
	HWND m_hPreviewDisplayWindow;
	HICON m_hDisplayWindowIcon;

	ColorGroup m_CenterGroup[NUM_COLORS];
	ColorGroup m_SurroundingGroup[NUM_COLORS];

	HFONT m_hDisplayFont;
	COLORREF m_TextColor;

	COLORREF m_DefaultCenterColor;
	COLORREF m_DefaultSurroundingColor;

	DisplayColoursDialogPersistentSettings *m_pdcdps;
};