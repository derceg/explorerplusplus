// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalControlApi.h"
#include <bit>

TerminalControlApi &TerminalControlApi::GetInstance()
{
	static TerminalControlApi api;
	return api;
}

bool TerminalControlApi::IsAvailable() const
{
	return m_module != nullptr;
}

template <typename T>
bool TerminalControlApi::Resolve(T &function, const char *name)
{
	function = std::bit_cast<T>(GetProcAddress(m_module.get(), name));
	return function != nullptr;
}

TerminalControlApi::TerminalControlApi()
{
	std::wstring applicationPath(MAX_PATH, L'\0');
	DWORD length = GetModuleFileName(nullptr, applicationPath.data(),
		static_cast<DWORD>(applicationPath.size()));

	if (length == 0 || length == applicationPath.size())
	{
		return;
	}

	applicationPath.resize(length);
	auto dllPath =
		std::filesystem::path(applicationPath).parent_path() / L"Microsoft.Terminal.Control.dll";

	m_module.reset(LoadLibraryEx(dllPath.c_str(), nullptr,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32));

	if (!m_module)
	{
		// LOAD_LIBRARY_SEARCH_* isn't supported on unpatched Windows 7. The path is absolute,
		// so this fallback still loads the intended DLL.
		m_module.reset(LoadLibrary(dllPath.c_str()));
	}

	if (!m_module)
	{
		LOG(WARNING) << "Microsoft.Terminal.Control.dll could not be loaded";
		return;
	}

	bool resolved = Resolve(createTerminal, "CreateTerminal")
		&& Resolve(destroyTerminal, "DestroyTerminal") && Resolve(sendOutput, "TerminalSendOutput")
		&& Resolve(registerScrollCallback, "TerminalRegisterScrollCallback")
		&& Resolve(registerWriteCallback, "TerminalRegisterWriteCallback")
		&& Resolve(userScroll, "TerminalUserScroll")
		&& Resolve(triggerResize, "TerminalTriggerResize")
		&& Resolve(dpiChanged, "TerminalDpiChanged")
		&& Resolve(sendKeyEvent, "TerminalSendKeyEvent")
		&& Resolve(sendCharEvent, "TerminalSendCharEvent")
		&& Resolve(getSelection, "TerminalGetSelection")
		&& Resolve(isSelectionActive, "TerminalIsSelectionActive")
		&& Resolve(setFocus, "TerminalSetFocus") && Resolve(killFocus, "TerminalKillFocus")
		&& Resolve(setTheme, "TerminalSetTheme");

	if (!resolved)
	{
		LOG(WARNING) << "Microsoft.Terminal.Control.dll has an incompatible API";
		m_module.reset();
	}
}
