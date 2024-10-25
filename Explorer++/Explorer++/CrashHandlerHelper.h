// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

struct CrashedData
{
	DWORD processId;
	DWORD threadId;
	intptr_t exceptionPointersAddress;
	std::string eventName;
};

void InitializeCrashHandler();
void HandleProcessCrashedNotification(const CrashedData &crashedData);
