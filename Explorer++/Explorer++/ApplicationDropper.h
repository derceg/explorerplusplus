// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/com.h>
#include <ShObjIdl.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Applications
{

class Application;
class ApplicationExecutor;
class ApplicationModel;

// Handles drag and drop for an applications view, where the view is target of the drop. When a drop
// occurs over an application, that application will be executed, with the dropped items passed as
// parameters. When a drop occurs over an index, the dropped items will be added as applications,
// starting from that index.
class ApplicationDropper
{
public:
	// Represents a drop target during a drag over an applications view. The target can either be an
	// existing application, or an index.
	class DropTarget
	{
	public:
		static DropTarget CreateForDropOnApplication(const Application *application);
		static DropTarget CreateForDropAtIndex(size_t index);

		const Application *GetApplication() const;
		std::optional<size_t> GetIndex() const;

	private:
		DropTarget(const Application *application);
		DropTarget(size_t index);

		const Application *m_application = nullptr;
		std::optional<size_t> m_index;
	};

	ApplicationDropper(IDataObject *dataObject, DWORD allowedEffects, ApplicationModel *model,
		ApplicationExecutor *applicationExecutor);
	~ApplicationDropper();

	DWORD GetDropEffect(const DropTarget &dropTarget);
	DWORD PerformDrop(const DropTarget &dropTarget);

private:
	struct ExtractedInfo
	{
		// If the drop occurs over an existing application, the paths stored here will be passed as
		// extra parameters to the application.
		std::vector<std::wstring> itemPaths;

		// If the drop occurs over an index, these applications wil be added to the model at that
		// index.
		std::vector<std::unique_ptr<Application>> applications;
	};

	ExtractedInfo &GetExtractedInfo();
	ExtractedInfo ExtractInfoFromShellItems();
	std::unique_ptr<Application> MaybeBuildApplicationFromShellItem(IShellItem *shellItem);
	void DropItemsOnApplication(const Application *targetApplication);
	void AddDropItems(size_t startingIndex);

	wil::com_ptr_nothrow<IDataObject> m_dataObject;
	DWORD m_allowedEffects;
	ApplicationModel *m_model = nullptr;
	ApplicationExecutor *m_applicationExecutor = nullptr;
	std::optional<ExtractedInfo> m_extractedInfo;
};

}
