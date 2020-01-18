// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"

namespace NSetFileAttributesDialogExternal
{
	struct SetFileAttributesInfo_t
	{
		TCHAR			szFullFileName[MAX_PATH];
		WIN32_FIND_DATA	wfd;
	};
}

class SetFileAttributesDialog;

class SetFileAttributesDialogPersistentSettings : public DialogSettings
{
public:

	static SetFileAttributesDialogPersistentSettings &GetInstance();

private:

	friend SetFileAttributesDialog;

	static const TCHAR SETTINGS_KEY[];

	SetFileAttributesDialogPersistentSettings();

	SetFileAttributesDialogPersistentSettings(const SetFileAttributesDialogPersistentSettings &);
	SetFileAttributesDialogPersistentSettings & operator=(const SetFileAttributesDialogPersistentSettings &);
};

class SetFileAttributesDialog : public BaseDialog
{
public:

	SetFileAttributesDialog(HINSTANCE hInstance, HWND hParent,
		std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList);

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnNotify(NMHDR *pnmhdr);
	INT_PTR	OnClose();

	void	SaveState();

private:

	typedef struct
	{
		DWORD Attribute;
		UINT uControlId;
		UINT uChecked;

		/* Set if the attribute state is on
		when the corresponding GUI element is
		'off'. */
		BOOL bReversed;
	} Attribute_t;

	enum DateTimeType_t
	{
		DATE_TIME_MODIFIED,
		DATE_TIME_CREATED,
		DATE_TIME_ACCESSED
	};

	void	InitializeAttributesStructure(void);

	void	ResetButtonState(HWND hwnd,BOOL bReset);
	void	SetAttributeCheckState(HWND hwnd,int nAttributes,int nSelected);

	void	InitializeDateFields();
	void	OnDateReset(DateTimeType_t DateTimeType);
	void	OnOk();
	void	OnCancel();

	std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t>	m_FileList;
	std::list<Attribute_t>	m_AttributeList;

	SetFileAttributesDialogPersistentSettings	*m_psfadps;

	SYSTEMTIME m_LocalWrite;
	SYSTEMTIME m_LocalCreation;
	SYSTEMTIME m_LocalAccess;

	BOOL	m_bModificationDateEnabled;
	BOOL	m_bCreationDateEnabled;
	BOOL	m_bAccessDateEnabled;
};