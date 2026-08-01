// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <boost/core/noncopyable.hpp>
#include <functional>
#include <optional>
#include <string>

class TabContent : private boost::noncopyable
{
public:
	enum class Icon
	{
		CommandLine
	};

	enum class MessageResult
	{
		NotHandled,
		Handled,
		BypassAccelerator
	};

	explicit TabContent(HWND window);
	virtual ~TabContent();

	HWND GetHWND() const;
	void AttachToTab(HWND replacedWindow);
	virtual void SetBounds(int x, int y, int width, int height, bool visible);
	virtual void Focus();
	virtual std::optional<std::wstring> GetName() const;
	virtual std::optional<std::wstring> GetTooltipText() const;
	virtual std::optional<Icon> GetIcon() const;
	virtual MessageResult ProcessMessage(const MSG *msg);

	void SetUpdatedCallback(std::function<void()> updatedCallback);

protected:
	void NotifyUpdated();

private:
	const HWND m_window;
	std::function<void()> m_updatedCallback;
};
