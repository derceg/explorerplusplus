// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/signals2.hpp>
#include <string>

namespace Applications
{

class Application
{
public:
	using UpdatedSignal = boost::signals2::signal<void(Application *application)>;

	Application(const std::wstring &name, const std::wstring &command,
		bool showNameOnToolbar = true);

	std::wstring GetName() const;
	void SetName(const std::wstring &name);
	std::wstring GetCommand() const;
	void SetCommand(const std::wstring &command);
	bool GetShowNameOnToolbar() const;
	void SetShowNameOnToolbar(bool showNameOnToolbar);

	boost::signals2::connection AddUpdatedObserver(const UpdatedSignal::slot_type &observer);

private:
	std::wstring m_name;
	std::wstring m_command;
	bool m_showNameOnToolbar;

	UpdatedSignal m_updatedSignal;
};

}
