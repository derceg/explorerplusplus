// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "TabContent.h"
#include "TerminalLaunchRequest.h"
#include <boost/core/noncopyable.hpp>
#include <boost/signals2/connection.hpp>
#include <functional>
#include <memory>
#include <optional>

struct Config;
class ClipboardStore;
class TerminalClipboard;
class TerminalHost;

class TerminalTabContent : public TabContent
{
public:
	static std::unique_ptr<TerminalTabContent> Create(HWND parent,
		const TerminalLaunchRequest &launchRequest, const Config *config,
		ClipboardStore *clipboardStore);

	~TerminalTabContent();

	void SetBounds(int x, int y, int width, int height, bool visible) override;
	void Focus() override;
	std::optional<std::wstring> GetName() const override;
	std::optional<std::wstring> GetTooltipText() const override;
	std::optional<Icon> GetIcon() const override;
	MessageResult ProcessMessage(const MSG *msg) override;

	void SetCloseRequestedCallback(std::function<void()> closeRequestedCallback);

private:
	TerminalTabContent(std::unique_ptr<TerminalHost> terminalHost,
		const TerminalLaunchRequest &launchRequest, const Config *config,
		ClipboardStore *clipboardStore);

	void UpdateFontSize();
	void OnDirectoryChanged(const std::wstring &directory);
	void OnProcessExited();

	std::unique_ptr<TerminalHost> m_terminalHost;
	std::unique_ptr<TerminalClipboard> m_terminalClipboard;
	const Config *const m_config;
	std::optional<std::wstring> m_directory;
	std::function<void()> m_closeRequestedCallback;
	bool m_startAttempted = false;
	bool m_closeRequestPending = false;
	boost::signals2::scoped_connection m_mainFontConnection;
};
