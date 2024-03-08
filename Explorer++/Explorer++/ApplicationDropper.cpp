// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ApplicationDropper.h"
#include "Application.h"
#include "ApplicationExecutor.h"
#include "ApplicationHelper.h"
#include "ApplicationModel.h"
#include <boost/algorithm/string/join.hpp>
#include <wil/com.h>
#include <wil/resource.h>

namespace Applications
{

ApplicationDropper::DropTarget ApplicationDropper::DropTarget::CreateForDropOnApplication(
	const Application *application)
{
	return DropTarget(application);
}

ApplicationDropper::DropTarget ApplicationDropper::DropTarget::CreateForDropAtIndex(size_t index)
{
	return DropTarget(index);
}

const Application *ApplicationDropper::DropTarget::GetApplication() const
{
	return m_application;
}

std::optional<size_t> ApplicationDropper::DropTarget::GetIndex() const
{
	return m_index;
}

ApplicationDropper::DropTarget::DropTarget(const Application *application) :
	m_application(application)
{
}

ApplicationDropper::DropTarget::DropTarget(size_t index) : m_index(index)
{
}

ApplicationDropper::ApplicationDropper(IDataObject *dataObject, DWORD allowedEffects,
	ApplicationModel *model, ApplicationExecutor *applicationExecutor) :
	m_dataObject(dataObject),
	m_allowedEffects(allowedEffects),
	m_model(model),
	m_applicationExecutor(applicationExecutor)
{
}

ApplicationDropper::~ApplicationDropper() = default;

DWORD ApplicationDropper::GetDropEffect(const DropTarget &dropTarget)
{
	auto &extractedInfo = GetExtractedInfo();

	if ((dropTarget.GetApplication() && extractedInfo.itemPaths.empty())
		|| (dropTarget.GetIndex() && extractedInfo.applications.empty()))
	{
		return DROPEFFECT_NONE;
	}

	if (WI_IsFlagSet(m_allowedEffects, DROPEFFECT_COPY))
	{
		return DROPEFFECT_COPY;
	}
	else if (WI_IsFlagSet(m_allowedEffects, DROPEFFECT_LINK))
	{
		return DROPEFFECT_LINK;
	}
	else
	{
		// In this case, the only allowed effect must be DROPEFFECT_MOVE. It's not possible to move
		// items to the applications toolbar.
		return DROPEFFECT_NONE;
	}
}

DWORD ApplicationDropper::PerformDrop(const DropTarget &dropTarget)
{
	DWORD targetEffect = GetDropEffect(dropTarget);

	if (targetEffect == DROPEFFECT_NONE)
	{
		return DROPEFFECT_NONE;
	}

	if (dropTarget.GetApplication())
	{
		DropItemsOnApplication(dropTarget.GetApplication());
	}
	else
	{
		AddDropItems(*dropTarget.GetIndex());
	}

	return targetEffect;
}

ApplicationDropper::ExtractedInfo &ApplicationDropper::GetExtractedInfo()
{
	if (!m_extractedInfo)
	{
		m_extractedInfo = ExtractInfoFromShellItems();
	}

	return *m_extractedInfo;
}

ApplicationDropper::ExtractedInfo ApplicationDropper::ExtractInfoFromShellItems()
{
	wil::com_ptr_nothrow<IShellItemArray> dropShellItems;
	HRESULT hr =
		SHCreateShellItemArrayFromDataObject(m_dataObject.get(), IID_PPV_ARGS(&dropShellItems));

	if (FAILED(hr))
	{
		return {};
	}

	DWORD numItems;
	hr = dropShellItems->GetCount(&numItems);

	if (FAILED(hr))
	{
		return {};
	}

	ExtractedInfo extractedInfo;

	for (DWORD i = 0; i < numItems; i++)
	{
		wil::com_ptr_nothrow<IShellItem> shellItem;
		hr = dropShellItems->GetItemAt(i, &shellItem);

		if (FAILED(hr))
		{
			continue;
		}

		wil::unique_cotaskmem_string parsingPath;
		hr = shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath);

		if (SUCCEEDED(hr))
		{
			extractedInfo.itemPaths.emplace_back(parsingPath.get());
		}

		auto application = MaybeBuildApplicationFromShellItem(shellItem.get());

		if (application)
		{
			extractedInfo.applications.push_back(std::move(application));
		}
	}

	return extractedInfo;
}

std::unique_ptr<Application> ApplicationDropper::MaybeBuildApplicationFromShellItem(
	IShellItem *shellItem)
{
	SFGAOF attributes;
	HRESULT hr = shellItem->GetAttributes(SFGAO_FOLDER, &attributes);

	// If the return value is S_OK, the returned attributes exactly match those passed in (i.e. the
	// item is a folder).
	if (hr == S_OK || FAILED(hr))
	{
		return nullptr;
	}

	wil::unique_cotaskmem_string displayName;
	hr = shellItem->GetDisplayName(SIGDN_NORMALDISPLAY, &displayName);

	if (FAILED(hr))
	{
		return nullptr;
	}

	auto nameWithoutExtension = ApplicationHelper::RemoveExtensionFromFileName(displayName.get());

	wil::unique_cotaskmem_string parsingPath;
	hr = shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parsingPath);

	if (FAILED(hr))
	{
		return nullptr;
	}

	std::wstring quotedParsingPath = parsingPath.get();

	if (quotedParsingPath.find(' ') != std::wstring::npos)
	{
		quotedParsingPath = L"\"" + quotedParsingPath + L"\"";
	}

	return std::make_unique<Application>(nameWithoutExtension, quotedParsingPath);
}

void ApplicationDropper::DropItemsOnApplication(const Application *targetApplication)
{
	auto &extractedInfo = GetExtractedInfo();

	std::vector<std::wstring> extraParametersVector = extractedInfo.itemPaths;
	std::transform(extraParametersVector.begin(), extraParametersVector.end(),
		extraParametersVector.begin(),
		[](const std::wstring &parameter) { return L"\"" + parameter + L"\""; });
	std::wstring extraParameters = boost::algorithm::join(extraParametersVector, L" ");

	m_applicationExecutor->Execute(targetApplication, extraParameters);
}

void ApplicationDropper::AddDropItems(size_t startingIndex)
{
	auto &extractedInfo = GetExtractedInfo();
	size_t index = startingIndex;

	for (auto &application : extractedInfo.applications)
	{
		m_model->AddItem(std::move(application), index);

		index++;
	}
}

}
