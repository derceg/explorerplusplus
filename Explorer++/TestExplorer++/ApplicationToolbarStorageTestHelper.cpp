// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ApplicationToolbarStorageTestHelper.h"
#include "Application.h"
#include "ApplicationModel.h"

using namespace Applications;

namespace Applications
{

bool operator==(const Application &first, const Application &second)
{
	return first.GetName() == second.GetName() && first.GetCommand() == second.GetCommand()
		&& first.GetShowNameOnToolbar() == second.GetShowNameOnToolbar();
}

}

void BuildLoadSaveReferenceModel(ApplicationModel *model)
{
	model->AddItem(
		std::make_unique<Application>(L"notepad", L"C:\\Windows\\System32\\notepad.exe", true));
	model->AddItem(std::make_unique<Application>(L"cmd", L"C:\\Windows\\System32\\cmd.exe", false));
	model->AddItem(std::make_unique<Application>(L"regedit", L"C:\\Windows\\regedit.exe", true));
	model->AddItem(std::make_unique<Application>(L"word",
		L"\"C:\\Program Files\\Microsoft Office\\root\\Office16\\WINWORD.EXE\"", true));
}
