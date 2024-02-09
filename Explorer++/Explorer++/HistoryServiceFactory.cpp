// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "HistoryServiceFactory.h"
#include "HistoryService.h"

HistoryServiceFactory::~HistoryServiceFactory() = default;

HistoryServiceFactory *HistoryServiceFactory::GetInstance()
{
	if (!m_staticInstance)
	{
		m_staticInstance = new HistoryServiceFactory();
	}

	return m_staticInstance;
}

HistoryService *HistoryServiceFactory::GetHistoryService()
{
	if (!m_historyService)
	{
		m_historyService = std::make_unique<HistoryService>();
	}

	return m_historyService.get();
}
