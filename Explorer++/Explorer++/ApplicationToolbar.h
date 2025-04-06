// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ApplicationDropper.h"
#include "ApplicationExecutorImpl.h"
#include "ToolbarView.h"
#include "../Helper/DropTargetWindow.h"
#include <boost/signals2.hpp>
#include <wil/com.h>
#include <vector>

class App;
class CoreInterface;
struct MouseEvent;
class ThemeManager;

namespace Applications
{

class Application;
class ApplicationModel;
class ApplicationToolbarView;

class ApplicationToolbar : private DropTargetInternal
{
public:
	static ApplicationToolbar *Create(ApplicationToolbarView *view, ApplicationModel *model,
		App *app, CoreInterface *coreInterface, ThemeManager *themeManager);

	ApplicationToolbar(const ApplicationToolbar &) = delete;
	ApplicationToolbar(ApplicationToolbar &&) = delete;
	ApplicationToolbar &operator=(const ApplicationToolbar &) = delete;
	ApplicationToolbar &operator=(ApplicationToolbar &&) = delete;

	ApplicationToolbarView *GetView() const;

private:
	class DragData
	{
	public:
		DragData(IDataObject *dataObject, std::unique_ptr<ApplicationDropper> applicationDropper) :
			m_dataObject(dataObject),
			m_applicationDropper(std::move(applicationDropper))
		{
		}

		IDataObject *GetDataObject() const
		{
			return m_dataObject.get();
		}

		ApplicationDropper *GetApplicationDropper() const
		{
			return m_applicationDropper.get();
		}

	private:
		wil::com_ptr_nothrow<IDataObject> m_dataObject;
		std::unique_ptr<ApplicationDropper> m_applicationDropper;
	};

	ApplicationToolbar(ApplicationToolbarView *view, ApplicationModel *model, App *app,
		CoreInterface *coreInterface, ThemeManager *themeManager);

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

	ApplicationDropper::DropTarget DropLocationToTarget(
		const ToolbarView::DropLocation &dropLocation);
	DWORD OnDragOver(POINT pt);
	void ResetDropState();
	const DragData &GetDragData() const;

	ApplicationToolbarView *m_view;
	ApplicationModel *m_model;
	ApplicationExecutorImpl m_applicationExecutor;
	App *const m_app;
	CoreInterface *const m_coreInterface;
	ThemeManager *const m_themeManager;

	std::vector<boost::signals2::scoped_connection> m_connections;

	// Drag and drop
	winrt::com_ptr<DropTargetWindow> m_dropTargetWindow;
	std::optional<DragData> m_dragData;
};

}
