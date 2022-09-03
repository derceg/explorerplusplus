// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Application.h"

namespace Applications
{

Application::Application(const std::wstring &name, const std::wstring &command,
	bool showNameOnToolbar) :
	m_name(name),
	m_command(command),
	m_showNameOnToolbar(showNameOnToolbar)
{
}

std::wstring Application::GetName() const
{
	return m_name;
}

void Application::SetName(const std::wstring &name)
{
	if (name == m_name)
	{
		return;
	}

	m_name = name;

	m_updatedSignal(this);
}

std::wstring Application::GetCommand() const
{
	return m_command;
}

void Application::SetCommand(const std::wstring &command)
{
	if (command == m_command)
	{
		return;
	}

	m_command = command;

	m_updatedSignal(this);
}

bool Application::GetShowNameOnToolbar() const
{
	return m_showNameOnToolbar;
}

void Application::SetShowNameOnToolbar(bool showNameOnToolbar)
{
	if (showNameOnToolbar == m_showNameOnToolbar)
	{
		return;
	}

	m_showNameOnToolbar = showNameOnToolbar;

	m_updatedSignal(this);
}

boost::signals2::connection Application::AddUpdatedObserver(
	const UpdatedSignal::slot_type &observer)
{
	return m_updatedSignal.connect(observer);
}

}
