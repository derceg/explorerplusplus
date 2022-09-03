// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ToolbarView.h"
#include "../Helper/DropTargetWindow.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <vector>

class CoreInterface;
struct MouseEvent;

namespace Applications
{

class ApplicationModel;
class Application;
class ApplicationToolbarView;

class ApplicationToolbar : private DropTargetInternal
{
public:
	static ApplicationToolbar *Create(ApplicationToolbarView *view, ApplicationModel *model,
		CoreInterface *coreInterface);

	ApplicationToolbarView *GetView() const;

private:
	ApplicationToolbar(ApplicationToolbarView *view, ApplicationModel *model,
		CoreInterface *coreInterface);

	void Initialize();

	void AddButtons();
	void AddButton(Application *application, size_t index);

	void OnApplicationAdded(Application *application, size_t index);
	void OnApplicationUpdated(Application *application);
	void OnApplicationRemoved(const Application *application, size_t oldIndex);

	void OnButtonClicked(const Application *application, const MouseEvent &event);
	void OnButtonRightClicked(Application *application, const MouseEvent &event);

	void OnWindowDestroyed();

	DWORD DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;
	DWORD DragOver(DWORD keyState, POINT pt, DWORD effect) override;
	void DragLeave() override;
	DWORD Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect) override;

	void StoreDropShellItemArray(IDataObject *dataObject);
	DWORD GetDropEffect(const ToolbarView::DropLocation &target);
	DWORD PerformDrop(const ToolbarView::DropLocation &target);
	DWORD DropItemsOnButton(size_t target);
	DWORD AddDropItems(size_t startingIndex);
	HRESULT AddDropItem(IShellItem *shellItem, size_t index);
	void ResetDropState();

	ApplicationToolbarView *m_view;
	ApplicationModel *m_model;
	CoreInterface *m_coreInterface;

	std::vector<boost::signals2::scoped_connection> m_connections;

	// Drag and drop
	winrt::com_ptr<DropTargetWindow> m_dropTargetWindow;
	wil::com_ptr_nothrow<IShellItemArray> m_dropShellItems;
	std::optional<bool> m_areAllDropItemsFolders;
};

}
