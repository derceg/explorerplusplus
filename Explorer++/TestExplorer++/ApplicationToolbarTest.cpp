// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ApplicationToolbar.h"
#include "ApplicationExecutorMock.h"
#include "ApplicationModel.h"
#include "ApplicationToolbarView.h"
#include "BrowserTestBase.h"
#include "BrowserWindowFake.h"
#include "Config.h"
#include "ShellTestHelper.h"
#include "../Helper/DragDropHelper.h"
#include <boost/range/combine.hpp>
#include <gtest/gtest.h>

using namespace Applications;
using namespace testing;

class ApplicationToolbarTestBase : public BrowserTestBase
{
protected:
	ApplicationToolbarTestBase(std::vector<std::unique_ptr<Application>> initialApplications = {}) :
		m_applicationModel(CreateApplicationModel(std::move(initialApplications))),
		m_browser(AddBrowser()),
		m_applicationToolbarView(ApplicationToolbarView::Create(m_browser->GetHWND(), &m_config)),
		m_applicationToolbar(ApplicationToolbar::Create(m_applicationToolbarView,
			m_applicationModel.get(), &m_applicationExecutor, nullptr, nullptr, nullptr))
	{
	}

	static std::unique_ptr<ApplicationModel> CreateApplicationModel(
		std::vector<std::unique_ptr<Application>> initialApplications)
	{
		auto applicationModel = std::make_unique<ApplicationModel>();

		for (auto &application : initialApplications)
		{
			applicationModel->AddItem(std::move(application));
		}

		return applicationModel;
	}

	void VerifyToolbarButtons()
	{
		const auto &buttons = m_applicationToolbarView->GetButtons();
		const auto &applications = m_applicationModel->GetItems();
		ASSERT_EQ(buttons.size(), applications.size());

		// TODO: This should use std::views::zip once C++23 support is available.
		for (const auto &[button, application] : boost::combine(buttons, applications))
		{
			std::wstring expectedName =
				application->GetShowNameOnToolbar() ? application->GetName() : L"";
			EXPECT_EQ(button->GetText(), expectedName);
		}
	}

	Config m_config;
	std::unique_ptr<ApplicationModel> m_applicationModel;
	ApplicationExecutorMock m_applicationExecutor;

	BrowserWindowFake *const m_browser;
	ApplicationToolbarView *const m_applicationToolbarView;
	ApplicationToolbar *const m_applicationToolbar;
};

class ApplicationToolbarTest : public ApplicationToolbarTestBase
{
protected:
	ApplicationToolbarTest() : ApplicationToolbarTestBase(CreateInitialApplications())
	{
	}

	static std::vector<std::unique_ptr<Application>> CreateInitialApplications()
	{
		std::vector<std::unique_ptr<Application>> initialApplications;
		initialApplications.push_back(std::make_unique<Application>(L"Application 1", L"c:\\cmd1"));
		initialApplications.push_back(std::make_unique<Application>(L"Application 2", L"c:\\cmd2"));
		initialApplications.push_back(std::make_unique<Application>(L"Application 3", L"c:\\cmd3"));
		return initialApplications;
	}
};

TEST_F(ApplicationToolbarTest, InitialApplications)
{
	VerifyToolbarButtons();
}

TEST_F(ApplicationToolbarTest, AddApplications)
{
	m_applicationModel->AddItem(std::make_unique<Application>(L"Application 4", L"c:\\cmd4"), 0);
	m_applicationModel->AddItem(
		std::make_unique<Application>(L"Application 5", L"f:\\path\\to\\file"), 1);
	m_applicationModel->AddItem(
		std::make_unique<Application>(L"Application 6", L"g:\\programs\\app"));

	VerifyToolbarButtons();
}

TEST_F(ApplicationToolbarTest, UpdateApplications)
{
	auto *application1 = m_applicationModel->GetItemAtIndex(0);
	auto *application2 = m_applicationModel->GetItemAtIndex(1);

	application1->SetName(L"Updated application name");
	application2->SetShowNameOnToolbar(false);

	VerifyToolbarButtons();
}

TEST_F(ApplicationToolbarTest, RemoveApplications)
{
	m_applicationModel->RemoveItem(m_applicationModel->GetItemAtIndex(0));
	m_applicationModel->RemoveItem(m_applicationModel->GetItemAtIndex(1));

	VerifyToolbarButtons();
}

TEST_F(ApplicationToolbarTest, OpenOnClick)
{
	InSequence seq;

	for (const auto &application : m_applicationModel->GetItems())
	{
		EXPECT_CALL(m_applicationExecutor, Execute(application.get(), std::wstring()));
	}

	for (const auto &button : m_applicationToolbarView->GetButtons())
	{
		button->OnClicked(MouseEvent{ { 0, 0 }, false, false });
	}
}

class ApplicationToolbarDropTest : public ApplicationToolbarTestBase
{
protected:
	wil::com_ptr_nothrow<IDataObject> CreateDataObjectFromPaths(
		const std::vector<std::wstring> &paths)
	{
		wil::com_ptr_nothrow<IDataObject> dataObject;
		CreateDataObjectFromPaths(paths, dataObject);
		return dataObject;
	}

private:
	void CreateDataObjectFromPaths(const std::vector<std::wstring> &paths,
		wil::com_ptr_nothrow<IDataObject> &outputDataObject)
	{
		std::vector<PidlAbsolute> pidls;

		for (const auto &path : paths)
		{
			pidls.push_back(CreateSimplePidlForTest(path));
		}

		ASSERT_HRESULT_SUCCEEDED(CreateDataObjectForShellTransfer(pidls, &outputDataObject));
	}
};

TEST_F(ApplicationToolbarDropTest, DropShellItems)
{
	std::vector<std::wstring> droppedItemPaths = { L"c:\\dropped-item1", L"h:\\dropped-item2" };
	auto dataObject = CreateDataObjectFromPaths(droppedItemPaths);

	POINT dropPt = { 0, 0 };
	ASSERT_TRUE(ClientToScreen(m_applicationToolbarView->GetHWND(), &dropPt));

	auto effect = m_applicationToolbar->SimulateDropForTest(dataObject.get(), MK_LBUTTON, dropPt,
		DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK);
	ASSERT_EQ(effect, static_cast<DWORD>(DROPEFFECT_COPY));

	// Each of the dropped items should be added to the model.
	const auto &applications = m_applicationModel->GetItems();
	ASSERT_EQ(applications.size(), droppedItemPaths.size());

	// TODO: This should use std::views::zip once C++23 support is available.
	for (const auto &[application, droppedItemPath] :
		boost::combine(applications, droppedItemPaths))
	{
		auto pidl = CreateSimplePidlForTest(droppedItemPath);

		std::wstring name;
		ASSERT_HRESULT_SUCCEEDED(GetDisplayName(pidl.Raw(), SHGDN_NORMAL, name));
		EXPECT_EQ(application->GetName(), name);

		EXPECT_THAT(application->GetCommand(), StrCaseEq(droppedItemPath));
	}
}
