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

// Provides a degree of abstraction off a standard dialog. For instance, provides the ability for a
// class to manage a dialog without having to handle the dialog procedure directly.
//
// A derived instance should only be allocated with new, since this class will destroy the instance
// automatically in WM_NCDESTROY. In practice, that means that a derived class should have a private
// constructor and private destructor, with instance creation provided via a static helper method.
class BaseDialog : public MessageForwarder, private boost::noncopyable
{
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

	INT_PTR ShowModalDialog();
	HWND ShowModelessDialog(std::function<void()> dialogDestroyedObserver);

protected:
	BaseDialog(const ResourceLoader *resourceLoader, int iResource, HWND hParent,
		DialogSizingType dialogSizingType);
	virtual ~BaseDialog() = default;

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
	INT_PTR OnNcDestroy() override final;

	const int m_iResource;
	const HWND m_hParent;
	std::function<void()> m_modelessDialogDestroyedObserver;

	wil::unique_hicon m_icon;

	bool m_showingModelessDialog = false;

	const DialogSizingType m_dialogSizingType;
	std::unique_ptr<ResizableDialogHelper> m_resizableDialogHelper;
};
