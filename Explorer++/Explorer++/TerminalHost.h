// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <functional>
#include <memory>
#include <string>

class TerminalHost : private boost::noncopyable
{
public:
	static std::unique_ptr<TerminalHost> Create(HWND parent, const std::wstring &directory,
		const std::wstring &initialCommand);

	~TerminalHost();

	HWND GetHWND() const;
	void SetBounds(int x, int y, int width, int height, bool visible);
	void SetFontSize(int fontSize);
	void Focus();
	void SetDirectoryChangedCallback(
		std::function<void(const std::wstring &)> directoryChangedCallback);

private:
	class Impl;

	explicit TerminalHost(std::unique_ptr<Impl> impl);

	const std::unique_ptr<Impl> m_impl;
};
