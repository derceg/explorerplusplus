// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "FrequentLocationsXmlStorage.h"
#include "FrequentLocationsModel.h"
#include "FrequentLocationsStorageTestHelper.h"
#include "ResourceTestHelper.h"
#include "XmlStorageTestHelper.h"
#include "../Helper/SystemClockImpl.h"
#include <gtest/gtest.h>

class FrequentLocationsXmlStorageTest : public XmlStorageTest
{
protected:
	SystemClockImpl m_systemClock;
};

TEST_F(FrequentLocationsXmlStorageTest, Load)
{
	FrequentLocationsModel referenceModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	std::wstring xmlFilePath = GetResourcePath(L"frequent-locations-config.xml");
	auto xmlDocumentData = LoadXmlDocument(xmlFilePath);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}

TEST_F(FrequentLocationsXmlStorageTest, Save)
{
	FrequentLocationsModel referenceModel(&m_systemClock);
	FrequentLocationsStorageTestHelper::BuildReferenceModel(&referenceModel);

	auto xmlDocumentData = CreateXmlDocument();

	FrequentLocationsXmlStorage::Save(xmlDocumentData.xmlDocument.get(),
		xmlDocumentData.rootNode.get(), &referenceModel);

	FrequentLocationsModel loadedModel(&m_systemClock);
	FrequentLocationsXmlStorage::Load(xmlDocumentData.rootNode.get(), &loadedModel);

	EXPECT_EQ(loadedModel, referenceModel);
}
