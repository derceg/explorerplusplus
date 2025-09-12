// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"

class RenameTabDialog;
class ResourceLoader;
class Tab;
class TabEvents;

class RenameTabDialogPersistentSettings : public DialogSettings
{
public:
	static RenameTabDialogPersistentSettings &GetInstance();

private:
	friend RenameTabDialog;

	static const TCHAR SETTINGS_KEY[];

	RenameTabDialogPersistentSettings();

	RenameTabDialogPersistentSettings(const RenameTabDialogPersistentSettings &);
	RenameTabDialogPersistentSettings &operator=(const RenameTabDialogPersistentSettings &);
};

class RenameTabDialog : public BaseDialog
{
public:
	static RenameTabDialog *Create(HWND parent, Tab *tab, TabEvents *tabEvents,
		const ResourceLoader *resourceLoader);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnClose() override;

	void SaveState() override;

private:
	RenameTabDialog(HWND parent, Tab *tab, TabEvents *tabEvents,
		const ResourceLoader *resourceLoader);
	~RenameTabDialog() = default;

	void OnUseFolderName();
	void OnUseCustomName();
	void OnOk();
	void OnCancel();

	void OnTabClosed(const Tab &tab);

	RenameTabDialogPersistentSettings *m_prtdps;

	Tab *const m_tab;

	std::vector<boost::signals2::scoped_connection> m_connections;
};
