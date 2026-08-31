// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "JumpListHelper.h"
#include "CommandLine.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Helper/ProcessHelper.h"
#include <propkey.h>
#include <propvarutil.h>
#include <string>
#include <vector>

namespace
{

struct JumpListTask
{
	std::wstring name;
	std::wstring path;
	std::wstring arguments;
	std::wstring iconPath;
	int iconIndex;
};

HRESULT AddJumpListTasks(const std::vector<JumpListTask> &tasks);
HRESULT AddJumpListTask(IObjectCollection *objectCollection, const JumpListTask &task);

}

void SetupJumpListTasks(const ResourceLoader *resourceLoader)
{
	auto processPath = GetCurrentProcessPath();

	JumpListTask task;
	task.name = resourceLoader->LoadString(IDS_TASKS_NEWTAB);
	task.path = processPath;
	task.arguments = CommandLine::JUMPLIST_TASK_NEWTAB_ARGUMENT;
	task.iconPath = processPath;
	task.iconIndex = 0;
	HRESULT hr = AddJumpListTasks({ task });
	DCHECK(SUCCEEDED(hr));
}

namespace
{

HRESULT AddJumpListTasks(const std::vector<JumpListTask> &tasks)
{
	wil::com_ptr_nothrow<ICustomDestinationList> customDestinationList;
	RETURN_IF_FAILED(CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&customDestinationList)));

	wil::com_ptr_nothrow<IObjectArray> removedItems;
	UINT minSlots;
	RETURN_IF_FAILED(customDestinationList->BeginList(&minSlots, IID_PPV_ARGS(&removedItems)));

	wil::com_ptr_nothrow<IObjectCollection> objectCollection;
	RETURN_IF_FAILED(CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr,
		CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&objectCollection)));

	for (const auto &task : tasks)
	{
		RETURN_IF_FAILED(AddJumpListTask(objectCollection.get(), task));
	}

	wil::com_ptr_nothrow<IObjectArray> items;
	RETURN_IF_FAILED(objectCollection->QueryInterface(IID_PPV_ARGS(&items)));
	RETURN_IF_FAILED(customDestinationList->AddUserTasks(items.get()));
	RETURN_IF_FAILED(customDestinationList->CommitList());

	return S_OK;
}

HRESULT AddJumpListTask(IObjectCollection *objectCollection, const JumpListTask &task)
{
	wil::com_ptr_nothrow<IShellLink> shellLink;
	RETURN_IF_FAILED(
		CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink)));
	RETURN_IF_FAILED(shellLink->SetPath(task.path.c_str()));
	RETURN_IF_FAILED(shellLink->SetArguments(task.arguments.c_str()));
	RETURN_IF_FAILED(shellLink->SetIconLocation(task.iconPath.c_str(), task.iconIndex));

	wil::com_ptr_nothrow<IPropertyStore> propertyStore;
	RETURN_IF_FAILED(shellLink->QueryInterface(IID_PPV_ARGS(&propertyStore)));

	wil::unique_prop_variant titleProperty;
	RETURN_IF_FAILED(InitPropVariantFromString(task.name.c_str(), &titleProperty));
	RETURN_IF_FAILED(propertyStore->SetValue(PKEY_Title, titleProperty));
	RETURN_IF_FAILED(propertyStore->Commit());

	RETURN_IF_FAILED(objectCollection->AddObject(shellLink.get()));

	return S_OK;
}

}
