// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TerminalLaunchRequest.h"
#include <boost/core/noncopyable.hpp>
#include <functional>
#include <memory>

class PseudoConsoleSession : private boost::noncopyable
{
public:
	using OutputCallback = std::function<void(const std::wstring &)>;

	static std::unique_ptr<PseudoConsoleSession> Create(const TerminalProcessLaunchInfo &launchInfo,
		OutputCallback outputCallback);

	~PseudoConsoleSession();

	void Resize(COORD size);
	void WriteInput(const std::wstring &input);
	void SetProcessExitedCallback(std::function<void()> processExitedCallback);

private:
	class Impl;

	explicit PseudoConsoleSession(std::unique_ptr<Impl> impl);

	const std::unique_ptr<Impl> m_impl;
};
