// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationToolbar.h"
#include "Application.h"
#include "ApplicationEditorDialog.h"
#include "ApplicationModel.h"
#include "ApplicationToolbarHelper.h"
#include "ApplicationToolbarView.h"
#include "CoreInterface.h"
#include "MainResource.h"
#include "ResourceHelper.h"
#include "../Helper/MenuHelper.h"
#include <boost/algorithm/string/join.hpp>
#include <glog/logging.h>
#include <propkey.h>

namespace Applications
{

using namespace ApplicationToolbarHelper;

class ApplicationToolbarButton : public ToolbarButton
{
public:
	ApplicationToolbarButton(const Application *application, ClickedCallback clickedCallback) :
		ToolbarButton(clickedCallback),
		m_application(application)
	{
	}

	std::wstring GetText() const override
	{
		if (m_application->GetShowNameOnToolbar())
		{
			return m_application->GetName();
		}

		return {};
	}

	std::wstring GetTooltipText() const override
	{
		return std::format(L"{}\n{}", m_application->GetName(), m_application->GetCommand());
	}

	std::optional<int> GetImageIndex() const override
	{
		ApplicationInfo applicationInfo = ParseCommandString(m_application->GetCommand());

		SHFILEINFO shfi;
		DWORD_PTR ret = SHGetFileInfo(applicationInfo.application.c_str(), 0, &shfi, sizeof(shfi),
			SHGFI_SYSICONINDEX);

		// Assign a generic icon if the file wasn't found.
		if (ret == 0)
		{
			return 0;
		}

		return shfi.iIcon;
	}

private:
	const Application *m_application;
};

ApplicationToolbar *ApplicationToolbar::Create(ApplicationToolbarView *view,
	ApplicationModel *model, CoreInterface *coreInterface)
{
	return new ApplicationToolbar(view, model, coreInterface);
}

ApplicationToolbar::ApplicationToolbar(ApplicationToolbarView *view, ApplicationModel *model,
	CoreInterface *coreInterface) :
	m_view(view),
	m_model(model),
	m_coreInterface(coreInterface),
	m_contextMenu(model, coreInterface)
{
	Initialize();
}

void ApplicationToolbar::Initialize()
{
	AddButtons();

	m_connections.push_back(m_model->AddItemAddedObserver(
		std::bind_front(&ApplicationToolbar::OnApplicationAdded, this)));
	m_connections.push_back(m_model->AddItemUpdatedObserver(
		std::bind_front(&ApplicationToolbar::OnApplicationUpdated, this)));
	m_connections.push_back(m_model->AddItemRemovedObserver(
		std::bind_front(&ApplicationToolbar::OnApplicationRemoved, this)));

	m_connections.push_back(m_coreInterface->AddToolbarContextMenuObserver(
		std::bind_front(&ApplicationToolbar::OnToolbarContextMenuPreShow, this)));
	m_connections.push_back(m_coreInterface->AddToolbarContextMenuSelectedObserver(
		std::bind_front(&ApplicationToolbar::OnToolbarContextMenuItemSelected, this)));

	m_dropTargetWindow = winrt::make_self<DropTargetWindow>(m_view->GetHWND(),
		static_cast<DropTargetInternal *>(this));

	m_view->AddWindowDestroyedObserver(
		std::bind_front(&ApplicationToolbar::OnWindowDestroyed, this));
}

ApplicationToolbarView *ApplicationToolbar::GetView() const
{
	return m_view;
}

void ApplicationToolbar::AddButtons()
{
	size_t index = 0;

	for (auto &application : m_model->GetItems())
	{
		AddButton(application.get(), index);
		++index;
	}
}

void ApplicationToolbar::AddButton(Application *application, size_t index)
{
	auto button = std::make_unique<ApplicationToolbarButton>(application,
		std::bind_front(&ApplicationToolbar::OnButtonClicked, this, application));
	button->SetRightClickedCallback(
		std::bind_front(&ApplicationToolbar::OnButtonRightClicked, this, application));

	m_view->AddButton(std::move(button), index);
}

void ApplicationToolbar::OnApplicationAdded(Application *application, size_t index)
{
	AddButton(application, index);
}

void ApplicationToolbar::OnApplicationUpdated(Application *application)
{
	// The ApplicationToolbarButton class holds a pointer to the associated Application (so there's
	// no need to update it). Only the view needs to be updated.
	auto index = m_model->GetItemIndex(application);
	m_view->UpdateButton(index);
}

void ApplicationToolbar::OnApplicationRemoved(const Application *application, size_t oldIndex)
{
	UNREFERENCED_PARAMETER(application);

	m_view->RemoveButton(oldIndex);
}

void ApplicationToolbar::OnButtonClicked(const Application *application, const MouseEvent &event)
{
	UNREFERENCED_PARAMETER(event);

	OpenApplication(m_coreInterface, m_view->GetHWND(), application);
}

void ApplicationToolbar::OnButtonRightClicked(Application *application, const MouseEvent &event)
{
	POINT ptScreen = event.ptClient;
	ClientToScreen(m_view->GetHWND(), &ptScreen);

	m_contextMenu.ShowMenu(m_view->GetHWND(), application, ptScreen);
}

void ApplicationToolbar::OnToolbarContextMenuPreShow(HMENU menu, HWND sourceWindow, const POINT &pt)
{
	UNREFERENCED_PARAMETER(pt);

	if (sourceWindow != m_view->GetHWND())
	{
		return;
	}

	std::wstring newText = ResourceHelper::LoadString(m_coreInterface->GetResourceInstance(),
		IDS_APPLICATIONBUTTON_NEW);
	MenuHelper::AddStringItem(menu, IDM_APP_NEW, newText, IDM_TOOLBARS_CUSTOMIZE, FALSE);
}

void ApplicationToolbar::OnToolbarContextMenuItemSelected(HWND sourceWindow, int menuItemId)
{
	if (sourceWindow != m_view->GetHWND())
	{
		return;
	}

	switch (menuItemId)
	{
	case IDM_APP_NEW:
	{
		ApplicationEditorDialog editorDialog(m_view->GetHWND(),
			m_coreInterface->GetResourceInstance(), m_model,
			ApplicationEditorDialog::EditDetails::AddNewApplication(
				std::make_unique<Application>(L"", L"")));
		editorDialog.ShowModalDialog();
	}
	break;
	}
}

void ApplicationToolbar::OnWindowDestroyed()
{
	delete this;
}

// DropTargetInternal
DWORD ApplicationToolbar::DragEnter(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	StoreDropShellItemArray(dataObject);
	auto target = m_view->GetDropLocation(pt);
	return GetDropEffect(target);
}

DWORD ApplicationToolbar::DragOver(DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	auto target = m_view->GetDropLocation(pt);
	return GetDropEffect(target);
}

void ApplicationToolbar::DragLeave()
{
	ResetDropState();
}

DWORD ApplicationToolbar::Drop(IDataObject *dataObject, DWORD keyState, POINT pt, DWORD effect)
{
	UNREFERENCED_PARAMETER(dataObject);
	UNREFERENCED_PARAMETER(keyState);
	UNREFERENCED_PARAMETER(effect);

	if (!m_dropShellItems)
	{
		return DROPEFFECT_NONE;
	}

	auto target = m_view->GetDropLocation(pt);
	DWORD targetEffect = PerformDrop(target);

	ResetDropState();

	return targetEffect;
}

void ApplicationToolbar::StoreDropShellItemArray(IDataObject *dataObject)
{
	DCHECK(!m_dropShellItems);

	wil::com_ptr_nothrow<IShellItemArray> dropShellItems;
	HRESULT hr = SHCreateShellItemArrayFromDataObject(dataObject, IID_PPV_ARGS(&dropShellItems));

	if (SUCCEEDED(hr))
	{
		m_dropShellItems = dropShellItems;
	}
}

DWORD ApplicationToolbar::GetDropEffect(const ToolbarView::DropLocation &target)
{
	if (!m_dropShellItems)
	{
		return DROPEFFECT_NONE;
	}

	// Items of any type (file/folder) may be dropped on a button.
	if (target.onItem)
	{
		return DROPEFFECT_COPY;
	}

	if (!m_areAllDropItemsFolders)
	{
		SFGAOF attributes;
		HRESULT hr = m_dropShellItems->GetAttributes(SIATTRIBFLAGS_AND, SFGAO_FOLDER, &attributes);

		// If the return value is S_OK, that means the attributes returned exactly match the ones
		// requested (i.e. every item is a folder).
		if (hr == S_OK)
		{
			m_areAllDropItemsFolders = true;
		}
		else
		{
			// This branch will be taken if the items aren't all folders, or the call fails. Setting
			// a value of false here at least means the drop will be attempted if the call fails and
			// the items are then dropped on the toolbar background (rather than on an existing
			// button).
			m_areAllDropItemsFolders = false;
		}
	}

	if (*m_areAllDropItemsFolders)
	{
		return DROPEFFECT_NONE;
	}

	return DROPEFFECT_COPY;
}

DWORD ApplicationToolbar::PerformDrop(const ToolbarView::DropLocation &target)
{
	if (target.onItem)
	{
		return DropItemsOnButton(target.index);
	}
	else
	{
		return AddDropItems(target.index);
	}
}

DWORD ApplicationToolbar::DropItemsOnButton(size_t target)
{
	DWORD numItems;
	HRESULT hr = m_dropShellItems->GetCount(&numItems);

	if (FAILED(hr))
	{
		return DROPEFFECT_NONE;
	}

	std::vector<std::wstring> extraParametersVector;

	for (DWORD i = 0; i < numItems; i++)
	{
		wil::com_ptr_nothrow<IShellItem> shellItem;
		hr = m_dropShellItems->GetItemAt(i, &shellItem);

		if (FAILED(hr))
		{
			continue;
		}

		wil::unique_cotaskmem_string parsingPath;
		hr = shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath);

		if (FAILED(hr))
		{
			continue;
		}

		extraParametersVector.push_back(parsingPath.get());
	}

	std::transform(extraParametersVector.begin(), extraParametersVector.end(),
		extraParametersVector.begin(),
		[](const std::wstring &parameter) { return L"\"" + parameter + L"\""; });

	std::wstring extraParameters = boost::algorithm::join(extraParametersVector, L" ");

	auto application = m_model->GetItemAtIndex(target);
	OpenApplication(m_coreInterface, m_view->GetHWND(), application, extraParameters);

	return DROPEFFECT_COPY;
}

DWORD ApplicationToolbar::AddDropItems(size_t startingIndex)
{
	if (m_areAllDropItemsFolders && *m_areAllDropItemsFolders)
	{
		return DROPEFFECT_NONE;
	}

	DWORD numItems;
	HRESULT hr = m_dropShellItems->GetCount(&numItems);

	if (FAILED(hr))
	{
		return DROPEFFECT_NONE;
	}

	size_t index = startingIndex;

	for (DWORD i = 0; i < numItems; i++)
	{
		wil::com_ptr_nothrow<IShellItem> shellItem;
		hr = m_dropShellItems->GetItemAt(i, &shellItem);

		if (FAILED(hr))
		{
			continue;
		}

		hr = AddDropItem(shellItem.get(), index);

		if (FAILED(hr))
		{
			continue;
		}

		index++;
	}

	if (index == startingIndex)
	{
		// In this case, no items were actually copied.
		return DROPEFFECT_NONE;
	}

	return DROPEFFECT_COPY;
}

HRESULT ApplicationToolbar::AddDropItem(IShellItem *shellItem, size_t index)
{
	SFGAOF attributes;
	HRESULT hr = shellItem->GetAttributes(SFGAO_FOLDER, &attributes);

	// As with testing shell item array attributes, if the return value is S_OK, the returned
	// attributes exactly match those passed in (i.e. the item is a folder).
	if (hr == S_OK || FAILED(hr))
	{
		return hr;
	}

	wil::com_ptr_nothrow<IShellItem2> shellItem2;
	RETURN_IF_FAILED(shellItem->QueryInterface(IID_PPV_ARGS(&shellItem2)));

	wil::unique_cotaskmem_string displayName;
	RETURN_IF_FAILED(shellItem2->GetString(PKEY_ItemNameDisplayWithoutExtension, &displayName));

	wil::unique_cotaskmem_string parsingPath;
	RETURN_IF_FAILED(shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath));

	std::wstring quotedParsingPath = parsingPath.get();

	if (quotedParsingPath.find(' ') != std::wstring::npos)
	{
		quotedParsingPath = L"\"" + quotedParsingPath + L"\"";
	}

	auto application = std::make_unique<Application>(displayName.get(), quotedParsingPath, true);
	m_model->AddItem(std::move(application), index);

	return S_OK;
}

void ApplicationToolbar::ResetDropState()
{
	m_dropShellItems.reset();
	m_areAllDropItemsFolders.reset();
}

}
