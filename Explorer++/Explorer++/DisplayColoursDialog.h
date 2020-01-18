// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseDialog.h"
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
	DisplayColoursDialogPersistentSettings & operator=(const DisplayColoursDialogPersistentSettings &);
};

class DisplayColoursDialog : public BaseDialog
{
public:

	DisplayColoursDialog(HINSTANCE hInstance, HWND hParent, HWND hDisplayWindow,
		COLORREF DefaultCenterColor, COLORREF DefaultSurroundingColor);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnHScroll(HWND hwnd);
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();
	INT_PTR	OnDestroy();

	void	SaveState();

private:

	enum Color_t
	{
		COLOR_RED,
		COLOR_GREEN,
		COLOR_BLUE
	};

	struct ColorGroup_t
	{
		UINT	SliderId;
		UINT	EditId;
		Color_t	Color;
	};

	static const int	NUM_COLORS = 3;
	static const int	TICK_REQUENCY = 10;

	void			OnRestoreDefaults();
	void			OnChooseFont();
	void			OnHScroll();
	void			OnEnChange(UINT ControlID);

	void			OnOk();
	void			OnCancel();

	void			InitializeColorGroups();
	void			InitializeColorGroupControls(ColorGroup_t ColorGroup[NUM_COLORS]);
	void			SetColorGroupValues(ColorGroup_t ColorGroup[NUM_COLORS],COLORREF Color);
	void			InitializePreviewWindow();

	void			UpdateEditControlsFromSlider(ColorGroup_t ColorGroup[NUM_COLORS]);
	COLORREF		GetColorFromSliderGroup(ColorGroup_t ColorGroup[NUM_COLORS]);

	HWND			m_hDisplayWindow;
	HWND			m_hPreviewDisplayWindow;
	HICON			m_hDisplayWindowIcon;

	ColorGroup_t	m_CenterGroup[NUM_COLORS];
	ColorGroup_t	m_SurroundingGroup[NUM_COLORS];

	HFONT			m_hDisplayFont;
	COLORREF		m_TextColor;

	COLORREF		m_DefaultCenterColor;
	COLORREF		m_DefaultSurroundingColor;

	DisplayColoursDialogPersistentSettings	*m_pdcdps;
};