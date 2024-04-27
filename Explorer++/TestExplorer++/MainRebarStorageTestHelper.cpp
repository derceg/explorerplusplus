// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "MainRebarStorageTestHelper.h"
#include "MainRebarStorage.h"

std::vector<RebarBandStorageInfo> BuildMainRebarLoadSaveReference()
{
	std::vector<RebarBandStorageInfo> referenceRebarStorageInfo;
	referenceRebarStorageInfo.emplace_back(1, 257, 0);
	referenceRebarStorageInfo.emplace_back(2, 769, 26);
	referenceRebarStorageInfo.emplace_back(0, 769, 621);
	referenceRebarStorageInfo.emplace_back(4, 769, 651);
	referenceRebarStorageInfo.emplace_back(3, 768, 557);
	return referenceRebarStorageInfo;
}
