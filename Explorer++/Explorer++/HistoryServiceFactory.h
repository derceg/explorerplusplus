// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <memory>

class HistoryService;

class HistoryServiceFactory
{
public:
	static HistoryServiceFactory *GetInstance();

	HistoryService *GetHistoryService();

private:
	HistoryServiceFactory() = default;
	~HistoryServiceFactory();

	static inline HistoryServiceFactory *m_staticInstance = nullptr;

	std::unique_ptr<HistoryService> m_historyService;
};
