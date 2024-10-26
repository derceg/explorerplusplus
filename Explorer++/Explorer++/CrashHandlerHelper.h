// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct CrashedData
{
	DWORD processId;
	DWORD threadId;
	intptr_t exceptionPointersAddress;
	std::wstring eventName;

	// This is only used in tests.
	bool operator==(const CrashedData &) const = default;
};

void InitializeCrashHandler();
std::wstring FormatCrashedDataForCommandLine(const CrashedData &crashedData);
void HandleProcessCrashedNotification(const CrashedData &crashedData);
