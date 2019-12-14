// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "TabContainer.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

class CRenameTabDialog;

class CRenameTabDialogPersistentSettings : public CDialogSettings
{
public:

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

	CRenameTabDialog(HINSTANCE hInstance, HWND hParent, int tabId, TabContainer *tabContainer);

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

	TabContainer	*m_tabContainer;
	int		m_tabId;

	std::vector<boost::signals2::scoped_connection>	m_connections;
};