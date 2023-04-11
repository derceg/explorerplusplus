// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "ColorRule.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class ColorRuleObserverMock
{
public:
	ColorRuleObserverMock(ColorRule *colorRule)
	{
		colorRule->AddUpdatedObserver(
			std::bind_front(&ColorRuleObserverMock::OnColorRuleUpdated, this));
	}

	MOCK_METHOD(void, OnColorRuleUpdated, (ColorRule * colorRule));
};

class ColorRuleTest : public Test
{
protected:
	ColorRuleTest() :
		m_colorRule(L"C++ files", L"*.cpp", true, 0, RGB(0, 0, 128)),
		m_observer(&m_colorRule)
	{
	}

	ColorRule m_colorRule;
	ColorRuleObserverMock m_observer;
};

TEST_F(ColorRuleTest, Update)
{
	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule));
	m_colorRule.SetDescription(L"Read-only header files");

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule));
	m_colorRule.SetFilterPattern(L"*.h");

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule));
	m_colorRule.SetFilterPatternCaseInsensitive(false);

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule));
	m_colorRule.SetFilterAttributes(FILE_ATTRIBUTE_READONLY);

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule));
	m_colorRule.SetColor(RGB(44, 44, 44));
}

TEST_F(ColorRuleTest, UpdateWithoutChange)
{
	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule)).Times(0);
	m_colorRule.SetDescription(m_colorRule.GetDescription());

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule)).Times(0);
	m_colorRule.SetFilterPattern(m_colorRule.GetFilterPattern());

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule)).Times(0);
	m_colorRule.SetFilterPatternCaseInsensitive(m_colorRule.GetFilterPatternCaseInsensitive());

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule)).Times(0);
	m_colorRule.SetFilterAttributes(m_colorRule.GetFilterAttributes());

	EXPECT_CALL(m_observer, OnColorRuleUpdated(&m_colorRule)).Times(0);
	m_colorRule.SetColor(m_colorRule.GetColor());
}
