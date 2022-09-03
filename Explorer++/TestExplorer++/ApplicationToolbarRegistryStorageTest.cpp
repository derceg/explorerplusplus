// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "ApplicationToolbarRegistryStorage.h"
#include "Application.h"
#include "ApplicationModel.h"
#include "ApplicationToolbarStorageHelper.h"
#include "RegistryStorageHelper.h"
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
	ApplicationToolbarRegistryStorage::Load(APPLICATION_TEST_KEY, &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(ApplicationToolbarRegistryStorageTest, Save)
{
	ApplicationModel referenceModel;
	BuildLoadSaveReferenceModel(&referenceModel);

	ApplicationToolbarRegistryStorage::Save(APPLICATION_TEST_KEY, &referenceModel);

	ApplicationModel loadedModel;
	ApplicationToolbarRegistryStorage::Load(APPLICATION_TEST_KEY, &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
