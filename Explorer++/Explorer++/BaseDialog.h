// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Helper/MessageForwarder.h"
#include "../Helper/ResizableDialogHelper.h"
#include <boost/core/noncopyable.hpp>
#include <wil/resource.h>
#include <functional>

class ResourceLoader;

/* Provides a degree of abstraction off a standard dialog.
For instance, provides the ability for a class to manage
a dialog without having to handle the dialog procedure
directly. */
class BaseDialog : public MessageForwarder, private boost::noncopyable
{
	friend INT_PTR CALLBACK BaseDialogProcStub(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	enum class DialogSizingType
	{
		None,
		Horizontal,
		Vertical,
		Both
	};

	static const int RETURN_CANCEL = 0;
	static const int RETURN_OK = 1;

	virtual ~BaseDialog() = default;

	INT_PTR ShowModalDialog();
	HWND ShowModelessDialog(std::function<void()> dialogDestroyedObserver);

protected:
	BaseDialog(const ResourceLoader *resourceLoader, HINSTANCE resourceInstance, int iResource,
		HWND hParent, DialogSizingType dialogSizingType);

	virtual void OnInitDialogBase();

	HINSTANCE GetResourceInstance() const;
	virtual wil::unique_hicon GetDialogIcon(int iconWidth, int iconHeight) const;

	INT_PTR GetDefaultReturnValue(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	HWND m_hDlg;
	int m_iMinWidth;
	int m_iMinHeight;

	HWND m_tipWnd;

	const ResourceLoader *const m_resourceLoader;

private:
	INT_PTR CALLBACK BaseDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual void AddDynamicControls();
	virtual std::vector<ResizableDialogControl> GetResizableControls();
	virtual void SaveState();

	const HINSTANCE m_resourceInstance;
	const int m_iResource;
	const HWND m_hParent;
	std::function<void()> m_modelessDialogDestroyedObserver;

	wil::unique_hicon m_icon;

	bool m_showingModelessDialog = false;

	const DialogSizingType m_dialogSizingType;
	std::unique_ptr<ResizableDialogHelper> m_resizableDialogHelper;
};
