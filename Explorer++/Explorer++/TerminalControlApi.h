// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <cstdint>

struct TerminalSize
{
	int x;
	int y;
};

struct TerminalTheme
{
	COLORREF defaultBackground;
	COLORREF defaultForeground;
	COLORREF defaultSelectionBackground;
	std::uint32_t cursorStyle;
	COLORREF colorTable[16];
};

class TerminalControlApi
{
public:
	using ScrollCallback = void(__stdcall *)(int viewTop, int viewHeight, int bufferSize);
	using WriteCallback = void(__stdcall *)(wchar_t *data);
	using CreateTerminalFunction = HRESULT(__stdcall *)(HWND parent, HWND *window, void **terminal);
	using DestroyTerminalFunction = void(__stdcall *)(void *terminal);
	using SendOutputFunction = void(__stdcall *)(void *terminal, const wchar_t *data);
	using RegisterScrollCallbackFunction = void(
		__stdcall *)(void *terminal, ScrollCallback callback);
	using RegisterWriteCallbackFunction = void(__stdcall *)(void *terminal, WriteCallback callback);
	using UserScrollFunction = void(__stdcall *)(void *terminal, int viewTop);
	using TriggerResizeFunction = HRESULT(
		__stdcall *)(void *terminal, int width, int height, TerminalSize *dimensions);
	using DpiChangedFunction = void(__stdcall *)(void *terminal, int dpi);
	using SendKeyEventFunction = void(__stdcall *)(void *terminal, unsigned short virtualKey,
		unsigned short scanCode, unsigned short flags, bool keyDown);
	using SendCharEventFunction = void(__stdcall *)(void *terminal, wchar_t character,
		unsigned short scanCode, unsigned short flags);
	using GetSelectionFunction = const wchar_t *(__stdcall *) (void *terminal);
	using IsSelectionActiveFunction = bool(__stdcall *)(void *terminal);
	using FocusFunction = void(__stdcall *)(void *terminal);
	using SetThemeFunction = void(__stdcall *)(void *terminal, TerminalTheme theme,
		const wchar_t *fontFamily, short fontSize, int dpi);

	static TerminalControlApi &GetInstance();

	bool IsAvailable() const;

	CreateTerminalFunction createTerminal = nullptr;
	DestroyTerminalFunction destroyTerminal = nullptr;
	SendOutputFunction sendOutput = nullptr;
	RegisterScrollCallbackFunction registerScrollCallback = nullptr;
	RegisterWriteCallbackFunction registerWriteCallback = nullptr;
	UserScrollFunction userScroll = nullptr;
	TriggerResizeFunction triggerResize = nullptr;
	DpiChangedFunction dpiChanged = nullptr;
	SendKeyEventFunction sendKeyEvent = nullptr;
	SendCharEventFunction sendCharEvent = nullptr;
	GetSelectionFunction getSelection = nullptr;
	IsSelectionActiveFunction isSelectionActive = nullptr;
	FocusFunction setFocus = nullptr;
	FocusFunction killFocus = nullptr;
	SetThemeFunction setTheme = nullptr;

private:
	TerminalControlApi();

	template <typename T>
	bool Resolve(T &function, const char *name);

	wil::unique_hmodule m_module;
};
