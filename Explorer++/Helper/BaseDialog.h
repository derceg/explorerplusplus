// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "DpiCompatibility.h"
#include "Macros.h"
#include "MessageForwarder.h"
#include "ReferenceCount.h"
#include "ResizableDialog.h"
#include <wil/resource.h>

__interface IModelessDialogNotification : public IReferenceCount
{
	void OnModelessDialogDestroy(int iResource);
};

/* Provides a degree of abstraction off a standard dialog.
For instance, provides the ability for a class to manage
a dialog without having to handle the dialog procedure
directly. */
class BaseDialog : public MessageForwarder
{
	friend INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:

	enum DialogSizeConstraint
	{
		DIALOG_SIZE_CONSTRAINT_NONE,
		DIALOG_SIZE_CONSTRAINT_X,
		DIALOG_SIZE_CONSTRAINT_Y
	};

	static const int RETURN_CANCEL = 0;
	static const int RETURN_OK = 1;

	BaseDialog(HINSTANCE hInstance,int iResource,HWND hParent,bool bResizable);
	virtual ~BaseDialog() = default;

	INT_PTR			ShowModalDialog();
	HWND			ShowModelessDialog(IModelessDialogNotification *pmdn = NULL);

protected:

	HINSTANCE		GetInstance() const;
	virtual wil::unique_hicon	GetDialogIcon(int iconWidth, int iconHeight) const;

	INT_PTR			GetDefaultReturnValue(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void			AddTooltipForControl(int controlId, int stringResourceId);

	HWND m_hDlg;
	DpiCompatibility m_dpiCompat;
	int m_iMinWidth;
	int m_iMinHeight;

private:

	DISALLOW_COPY_AND_ASSIGN(BaseDialog);

	INT_PTR CALLBACK	BaseDialogProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	virtual void	GetResizableControlInformation(DialogSizeConstraint &dsc, std::list<ResizableDialog::Control_t> &controlList);
	virtual void	SaveState();

	const HINSTANCE m_hInstance;
	const int m_iResource;
	const HWND m_hParent;
	IModelessDialogNotification *m_pmdn;

	wil::unique_hicon m_icon;

	BOOL m_bShowingModelessDialog;

	/* Used only with resizable dialogs. */
	const bool m_bResizable;
	DialogSizeConstraint m_dsc;
	std::unique_ptr<ResizableDialog> m_prd;

	HWND m_tipWnd;
};