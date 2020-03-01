// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/BaseWindow.h"

__interface IExplorerplusplus;

class ListViewEdit : BaseWindow
{
public:

	static ListViewEdit *CreateNew(HWND hwnd,int ItemIndex,IExplorerplusplus *pexpp);

protected:

	void				OnEMSetSel(WPARAM &wParam,LPARAM &lParam) override;

	INT_PTR				OnPrivateMessage(UINT uMsg,WPARAM wParam,LPARAM lParam) override;

private:

	enum class RenameStage
	{
		Filename,
		Extension,
		Entire
	};

	ListViewEdit(HWND hwnd,int ItemIndex,IExplorerplusplus *pexpp);

	int					GetExtensionIndex();

	IExplorerplusplus	*m_pexpp;

	int					m_ItemIndex;
	RenameStage		m_RenameStage;
	bool				m_BeginRename;
};