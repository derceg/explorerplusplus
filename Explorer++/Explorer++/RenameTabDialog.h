// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "TabContainerInterface.h"
#include "TabInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

class CRenameTabDialog;

class CRenameTabDialogPersistentSettings : public CDialogSettings
{
public:

	~CRenameTabDialogPersistentSettings();

	static CRenameTabDialogPersistentSettings &GetInstance();

private:

	friend CRenameTabDialog;

	static const TCHAR SETTINGS_KEY[];

	CRenameTabDialogPersistentSettings();

	CRenameTabDialogPersistentSettings(const CRenameTabDialogPersistentSettings &);
	CRenameTabDialogPersistentSettings & operator=(const CRenameTabDialogPersistentSettings &);
};

class CRenameTabDialog : public CBaseDialog
{
public:

	CRenameTabDialog(HINSTANCE hInstance, int iResource, HWND hParent,
		int iTab, IExplorerplusplus *pexpp, TabContainerInterface *tabContainer, TabInterface *ti);
	~CRenameTabDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();

	void	SaveState();

private:

	void	OnUseFolderName();
	void	OnUseCustomName();
	void	OnOk();
	void	OnCancel();

	void	OnTabClosed(int tabId);

	CRenameTabDialogPersistentSettings	*m_prtdps;

	IExplorerplusplus	*m_pexpp;
	TabContainerInterface	*m_tabContainer;
	TabInterface	*m_ti;
	int		m_tabId;
	TCHAR	m_szTabName[256];

	boost::signals2::connection	m_tabRemovedConnection;
};