// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ApplicationToolbarRegistryStorage.h"
#include "Application.h"
#include "ApplicationModel.h"
#include "ApplicationToolbarStorageTestHelper.h"
#include "MovableModelHelper.h"
#include "RegistryStorageTestHelper.h"
#include <gtest/gtest.h>

using namespace Applications;
using namespace testing;

class ApplicationToolbarRegistryStorageTest : public RegistryStorageTest
{
};

TEST_F(ApplicationToolbarRegistryStorageTest, Load)
{
	ApplicationModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	ImportRegistryResource(L"application-toolbar.reg");

	ApplicationModel loadedModel;
	ApplicationToolbarRegistryStorage::Load(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(ApplicationToolbarRegistryStorageTest, Save)
{
	ApplicationModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	ApplicationToolbarRegistryStorage::Save(m_applicationTestKey.get(), &referenceModel);

	ApplicationModel loadedModel;
	ApplicationToolbarRegistryStorage::Load(m_applicationTestKey.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
