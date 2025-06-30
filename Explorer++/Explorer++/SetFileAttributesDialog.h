// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include <list>
#include <vector>

class SetFileAttributesDialog;

class SetFileAttributesDialogPersistentSettings : public DialogSettings
{
public:
	static SetFileAttributesDialogPersistentSettings &GetInstance();

private:
	friend SetFileAttributesDialog;

	static const TCHAR SETTINGS_KEY[];

	SetFileAttributesDialogPersistentSettings();

	SetFileAttributesDialogPersistentSettings(
		const SetFileAttributesDialogPersistentSettings &) = delete;
	SetFileAttributesDialogPersistentSettings &operator=(
		const SetFileAttributesDialogPersistentSettings &) = delete;
};

struct SetFileAttributesItem
{
	SetFileAttributesItem(const std::wstring &path, const WIN32_FIND_DATA &findData) :
		path(path),
		findData(findData)
	{
	}

	std::wstring path;
	WIN32_FIND_DATA findData;
};

class SetFileAttributesDialog : public BaseDialog
{
public:
	SetFileAttributesDialog(const ResourceLoader *resourceLoader, HWND hParent,
		const std::vector<SetFileAttributesItem> &items);

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnNotify(NMHDR *pnmhdr) override;
	INT_PTR OnClose() override;

	void SaveState() override;

private:
	typedef struct
	{
		DWORD Attribute;
		UINT uControlId;
		UINT uChecked;
	} Attribute_t;

	enum class DateTimeType
	{
		Modified,
		Created,
		Accessed
	};

	void InitializeAttributesStructure();

	void ResetButtonState(HWND hwnd, BOOL bReset);
	void SetAttributeCheckState(HWND hwnd, int nAttributes, int nSelected);

	void InitializeDateFields();
	void OnDateReset(DateTimeType dateTimeType);
	void OnOk();
	void OnCancel();

	const std::vector<SetFileAttributesItem> m_items;
	std::list<Attribute_t> m_AttributeList;

	SetFileAttributesDialogPersistentSettings *m_psfadps;

	SYSTEMTIME m_LocalWrite;
	SYSTEMTIME m_LocalCreation;
	SYSTEMTIME m_LocalAccess;

	BOOL m_bModificationDateEnabled;
	BOOL m_bCreationDateEnabled;
	BOOL m_bAccessDateEnabled;
};
