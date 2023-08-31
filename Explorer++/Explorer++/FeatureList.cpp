// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "FeatureList.h"
#include "CommandLine.h"

FeatureList *FeatureList::GetInstance()
{
	if (!m_staticInstance)
	{
		m_staticInstance = new FeatureList();
	}

	return m_staticInstance;
}

void FeatureList::InitializeFromCommandLine(const CommandLine::Settings &commandLineSettings)
{
	assert(!m_initializedFromCommandLine);

	m_enabledFeatures = commandLineSettings.enableFeatures;

	m_initializedFromCommandLine = true;
}

bool FeatureList::IsEnabled(Feature feature)
{
	return m_enabledFeatures.find(feature) != m_enabledFeatures.end();
}
