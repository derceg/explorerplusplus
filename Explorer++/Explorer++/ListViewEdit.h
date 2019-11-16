// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "../Helper/BaseWindow.h"

class CListViewEdit : CBaseWindow
{
public:

	static CListViewEdit *CreateNew(HWND hwnd,int ItemIndex,IExplorerplusplus *pexpp);

protected:

	void				OnEMSetSel(WPARAM &wParam,LPARAM &lParam);

	INT_PTR				OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);

private:

	enum RenameStage_t
	{
		RENAME_FILENAME,
		RENAME_EXTENSION,
		RENAME_ENTIRE
	};

	CListViewEdit(HWND hwnd,int ItemIndex,IExplorerplusplus *pexpp);

	int					GetExtensionIndex();

	IExplorerplusplus	*m_pexpp;

	int					m_ItemIndex;
	RenameStage_t		m_RenameStage;
	bool				m_BeginRename;
};