// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ApplicationDropper.h"
#include "ApplicationExecutorMock.h"
#include "ApplicationHelper.h"
#include "ApplicationModel.h"
#include "DragDropTestHelper.h"
#include "../Helper/ShellHelper.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wil/com.h>

using namespace Applications;
using namespace testing;

// Tests dropping a shell item.
class ApplicationDropperShellItemTest : public TestWithParam<ShellItemType>
{
protected:
	void SetUp() override
	{
		m_application = m_applicationModel.AddItem(
			std::make_unique<Application>(L"notepad", L"C:\\Windows\\System32\\notepad.exe"));

		m_itemName = L"item.txt";
		m_itemPath = L"c:\\path\\to\\" + m_itemName;

		CreateShellDataObject(m_itemPath, GetParam(), m_dataObject);
	}

	void CheckDropEffect(DWORD allowedEffects, DWORD expectedEffect,
		const ApplicationDropper::DropTarget &dropTarget)
	{
		auto dropper = std::make_unique<ApplicationDropper>(m_dataObject.get(), allowedEffects,
			&m_applicationModel, &m_applicationExecutor);
		DWORD effect = dropper->GetDropEffect(dropTarget);
		EXPECT_EQ(effect, expectedEffect);
	}

	ApplicationModel m_applicationModel;
	Application *m_application = nullptr;
	ApplicationExecutorMock m_applicationExecutor;
	wil::com_ptr_nothrow<IDataObject> m_dataObject;
	std::wstring m_itemName;
	std::wstring m_itemPath;
};

TEST_P(ApplicationDropperShellItemTest, DropAtIndexEffect)
{
	auto dropTarget = ApplicationDropper::DropTarget::CreateForDropAtIndex(0);

	CheckDropEffect(DROPEFFECT_COPY,
		GetParam() == ShellItemType::File ? DROPEFFECT_COPY : DROPEFFECT_NONE, dropTarget);
	CheckDropEffect(DROPEFFECT_LINK,
		GetParam() == ShellItemType::File ? DROPEFFECT_LINK : DROPEFFECT_NONE, dropTarget);
	CheckDropEffect(DROPEFFECT_MOVE, DROPEFFECT_NONE, dropTarget);
}

TEST_P(ApplicationDropperShellItemTest, DropOnApplicationEffect)
{
	auto dropTarget = ApplicationDropper::DropTarget::CreateForDropOnApplication(m_application);

	CheckDropEffect(DROPEFFECT_COPY, DROPEFFECT_COPY, dropTarget);
	CheckDropEffect(DROPEFFECT_LINK, DROPEFFECT_LINK, dropTarget);
	CheckDropEffect(DROPEFFECT_MOVE, DROPEFFECT_NONE, dropTarget);
}

TEST_P(ApplicationDropperShellItemTest, DropAtIndex)
{
	auto dropper = std::make_unique<ApplicationDropper>(m_dataObject.get(), DROPEFFECT_COPY,
		&m_applicationModel, &m_applicationExecutor);

	DWORD effect = dropper->PerformDrop(ApplicationDropper::DropTarget::CreateForDropAtIndex(0));

	if (GetParam() == ShellItemType::Folder)
	{
		EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
		return;
	}

	ASSERT_EQ(effect, static_cast<DWORD>(DROPEFFECT_COPY));
	ASSERT_EQ(m_applicationModel.GetItems().size(), 2U);

	auto *application = m_applicationModel.GetItemAtIndex(0);
	EXPECT_EQ(application->GetName(), ApplicationHelper::RemoveExtensionFromFileName(m_itemName));
	EXPECT_THAT(application->GetCommand(), StrCaseEq(m_itemPath));
}

TEST_P(ApplicationDropperShellItemTest, DropOnApplication)
{
	auto dropper = std::make_unique<ApplicationDropper>(m_dataObject.get(), DROPEFFECT_COPY,
		&m_applicationModel, &m_applicationExecutor);

	EXPECT_CALL(m_applicationExecutor,
		Execute(m_application, StrCaseEq(L"\"" + m_itemPath + L"\"")));

	DWORD effect = dropper->PerformDrop(
		ApplicationDropper::DropTarget::CreateForDropOnApplication(m_application));
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_COPY));

	// The items were dropped on an existing application, so no new items should have been added.
	EXPECT_EQ(m_applicationModel.GetItems().size(), 1U);
}

INSTANTIATE_TEST_SUITE_P(FileAndFolder, ApplicationDropperShellItemTest,
	Values(ShellItemType::File, ShellItemType::Folder));

class ApplicationDropperInvalidDataTest : public Test
{
protected:
	void SetUp() override
	{
		m_application = m_applicationModel.AddItem(
			std::make_unique<Application>(L"notepad", L"C:\\Windows\\System32\\notepad.exe"));

		CreateTextDataObject(L"Test", m_dataObject);
		m_dropper = std::make_unique<ApplicationDropper>(m_dataObject.get(), DROPEFFECT_COPY,
			&m_applicationModel, &m_applicationExecutor);
	}

	ApplicationModel m_applicationModel;
	Application *m_application = nullptr;
	ApplicationExecutorMock m_applicationExecutor;
	winrt::com_ptr<IDataObject> m_dataObject;
	std::unique_ptr<ApplicationDropper> m_dropper;
};

TEST_F(ApplicationDropperInvalidDataTest, DropEffect)
{
	DWORD effect =
		m_dropper->GetDropEffect(ApplicationDropper::DropTarget::CreateForDropAtIndex(0));
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	effect = m_dropper->GetDropEffect(
		ApplicationDropper::DropTarget::CreateForDropOnApplication(m_application));
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}

TEST_F(ApplicationDropperInvalidDataTest, Drop)
{
	DWORD effect = m_dropper->PerformDrop(ApplicationDropper::DropTarget::CreateForDropAtIndex(0));
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));

	effect = m_dropper->PerformDrop(
		ApplicationDropper::DropTarget::CreateForDropOnApplication(m_application));
	EXPECT_EQ(effect, static_cast<DWORD>(DROPEFFECT_NONE));
}
