// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TerminalHost.h"
#include "../Helper/DpiCompatibility.h"
#include "../Helper/WindowSubclass.h"
#include <atomic>
#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string_view>
#include <thread>

namespace
{

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
	using WriteCallback = void(__stdcall *)(wchar_t *data);
	using CreateTerminalFunction = HRESULT(__stdcall *)(HWND parent, HWND *window, void **terminal);
	using DestroyTerminalFunction = void(__stdcall *)(void *terminal);
	using SendOutputFunction = void(__stdcall *)(void *terminal, const wchar_t *data);
	using RegisterWriteCallbackFunction = void(__stdcall *)(void *terminal, WriteCallback callback);
	using TriggerResizeFunction = HRESULT(
		__stdcall *)(void *terminal, int width, int height, TerminalSize *dimensions);
	using DpiChangedFunction = void(__stdcall *)(void *terminal, int dpi);
	using SendKeyEventFunction = void(__stdcall *)(void *terminal, unsigned short virtualKey,
		unsigned short scanCode, unsigned short flags, bool keyDown);
	using SendCharEventFunction = void(__stdcall *)(void *terminal, wchar_t character,
		unsigned short scanCode, unsigned short flags);
	using FocusFunction = void(__stdcall *)(void *terminal);
	using SetThemeFunction = void(__stdcall *)(void *terminal, TerminalTheme theme,
		const wchar_t *fontFamily, short fontSize, int dpi);

	static TerminalControlApi &GetInstance()
	{
		static TerminalControlApi api;
		return api;
	}

	bool IsAvailable() const
	{
		return m_module != nullptr;
	}

	CreateTerminalFunction createTerminal = nullptr;
	DestroyTerminalFunction destroyTerminal = nullptr;
	SendOutputFunction sendOutput = nullptr;
	RegisterWriteCallbackFunction registerWriteCallback = nullptr;
	TriggerResizeFunction triggerResize = nullptr;
	DpiChangedFunction dpiChanged = nullptr;
	SendKeyEventFunction sendKeyEvent = nullptr;
	SendCharEventFunction sendCharEvent = nullptr;
	FocusFunction setFocus = nullptr;
	FocusFunction killFocus = nullptr;
	SetThemeFunction setTheme = nullptr;

private:
	template <typename T>
	bool Resolve(T &function, const char *name)
	{
		function = std::bit_cast<T>(GetProcAddress(m_module.get(), name));
		return function != nullptr;
	}

	TerminalControlApi()
	{
		std::wstring applicationPath(MAX_PATH, L'\0');
		DWORD length = GetModuleFileName(nullptr, applicationPath.data(),
			static_cast<DWORD>(applicationPath.size()));

		if (length == 0 || length == applicationPath.size())
		{
			return;
		}

		applicationPath.resize(length);
		auto dllPath = std::filesystem::path(applicationPath).parent_path()
			/ L"Microsoft.Terminal.Control.dll";

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
			&& Resolve(destroyTerminal, "DestroyTerminal")
			&& Resolve(sendOutput, "TerminalSendOutput")
			&& Resolve(registerWriteCallback, "TerminalRegisterWriteCallback")
			&& Resolve(triggerResize, "TerminalTriggerResize")
			&& Resolve(dpiChanged, "TerminalDpiChanged")
			&& Resolve(sendKeyEvent, "TerminalSendKeyEvent")
			&& Resolve(sendCharEvent, "TerminalSendCharEvent")
			&& Resolve(setFocus, "TerminalSetFocus") && Resolve(killFocus, "TerminalKillFocus")
			&& Resolve(setTheme, "TerminalSetTheme");

		if (!resolved)
		{
			LOG(WARNING) << "Microsoft.Terminal.Control.dll has an incompatible API";
			m_module.reset();
		}
	}

	wil::unique_hmodule m_module;
};

class PseudoConsoleApi
{
public:
	using CreateFunction = HRESULT(
		WINAPI *)(COORD size, HANDLE input, HANDLE output, DWORD flags, HANDLE *pseudoConsole);
	using ResizeFunction = HRESULT(WINAPI *)(HANDLE pseudoConsole, COORD size);
	using CloseFunction = void(WINAPI *)(HANDLE pseudoConsole);

	static PseudoConsoleApi &GetInstance()
	{
		static PseudoConsoleApi api;
		return api;
	}

	bool IsAvailable() const
	{
		return create != nullptr && resize != nullptr && close != nullptr;
	}

	CreateFunction create = nullptr;
	ResizeFunction resize = nullptr;
	CloseFunction close = nullptr;

private:
	PseudoConsoleApi()
	{
		HMODULE kernel32 = GetModuleHandle(L"kernel32.dll");

		if (!kernel32)
		{
			return;
		}

		create = std::bit_cast<CreateFunction>(GetProcAddress(kernel32, "CreatePseudoConsole"));
		resize = std::bit_cast<ResizeFunction>(GetProcAddress(kernel32, "ResizePseudoConsole"));
		close = std::bit_cast<CloseFunction>(GetProcAddress(kernel32, "ClosePseudoConsole"));
	}
};

constexpr DWORD PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE_VALUE = 0x00020016;
constexpr COORD DEFAULT_TERMINAL_SIZE{ 80, 25 };

// TerminalSetTheme doesn't expose a font-only mode. Keep the control's standard Campbell colors
// and blinking bar cursor when changing the font size.
constexpr TerminalTheme DEFAULT_TERMINAL_THEME{ 0x0c0c0c, 0xcccccc, 0xcccccc, 5,
	{ 0x0c0c0c, 0x1f0fc5, 0x0ea113, 0x009cc1, 0xda3700, 0x981788, 0xdd963a, 0xcccccc, 0x767676,
		0x5648e7, 0x0cc616, 0xa5f1f9, 0xff783b, 0x9e00b4, 0xd6d661, 0xf2f2f2 } };
constexpr std::wstring_view TERMINAL_FONT_FAMILY = L"Cascadia Mono";
constexpr std::wstring_view CURRENT_DIRECTORY_SEQUENCE_PREFIX = L"\x1b]9;9;";
const UINT TERMINAL_DIRECTORY_CHANGED_MESSAGE =
	RegisterWindowMessage(L"Explorer++TerminalDirectoryChanged");

std::string ToUtf8(std::wstring_view input)
{
	if (input.empty())
	{
		return {};
	}

	int requiredSize = WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()),
		nullptr, 0, nullptr, nullptr);

	if (requiredSize == 0)
	{
		return {};
	}

	std::string output(requiredSize, '\0');
	WideCharToMultiByte(CP_UTF8, 0, input.data(), static_cast<int>(input.size()), output.data(),
		requiredSize, nullptr, nullptr);
	return output;
}

std::wstring QuoteArgument(const std::wstring &argument)
{
	return L"\"" + argument + L"\"";
}

}

class TerminalHost::Impl
{
public:
	static std::unique_ptr<Impl> Create(HWND parent, const std::wstring &directory)
	{
		auto &terminalApi = TerminalControlApi::GetInstance();
		auto &pseudoConsoleApi = PseudoConsoleApi::GetInstance();

		if (!terminalApi.IsAvailable() || !pseudoConsoleApi.IsAvailable())
		{
			return nullptr;
		}

		auto impl = std::unique_ptr<Impl>(new Impl());
		HRESULT result = terminalApi.createTerminal(parent, &impl->m_window, &impl->m_terminal);

		if (FAILED(result) || !impl->m_window || !impl->m_terminal)
		{
			LOG(WARNING) << "The Windows Terminal control could not be created";
			return nullptr;
		}

		impl->m_windowSubclass = std::make_unique<WindowSubclass>(impl->m_window,
			std::bind_front(&Impl::WindowProcedure, impl.get()));
		terminalApi.registerWriteCallback(impl->m_terminal, &Impl::OnTerminalWrite);
		impl->m_dpi = DpiCompatibility::GetInstance().GetDpiForWindow(parent);
		terminalApi.dpiChanged(impl->m_terminal, impl->m_dpi);

		if (!impl->StartPseudoConsole(directory))
		{
			return nullptr;
		}

		impl->m_outputThread = std::thread(&Impl::ReadOutput, impl.get());
		return impl;
	}

	~Impl()
	{
		{
			std::scoped_lock lock(s_activeHostMutex);

			if (s_activeHost == this)
			{
				s_activeHost = nullptr;
			}
		}

		m_inputWrite.reset();

		if (m_pseudoConsole)
		{
			PseudoConsoleApi::GetInstance().close(m_pseudoConsole);
			m_pseudoConsole = nullptr;
		}

		m_stopping = true;

		if (m_outputThread.joinable())
		{
			CancelSynchronousIo(m_outputThread.native_handle());
			m_outputThread.join();
		}

		m_outputRead.reset();

		if (m_process && WaitForSingleObject(m_process.get(), 0) == WAIT_TIMEOUT)
		{
			TerminateProcess(m_process.get(), 0);
			WaitForSingleObject(m_process.get(), 1000);
		}

		m_windowSubclass.reset();

		if (m_terminal)
		{
			TerminalControlApi::GetInstance().destroyTerminal(m_terminal);
		}
	}

	HWND GetHWND() const
	{
		return m_window;
	}

	void SetBounds(int x, int y, int width, int height, bool visible)
	{
		UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
		width = std::max(width, 1);
		height = std::max(height, 1);

		TerminalSize dimensions{};
		auto &terminalApi = TerminalControlApi::GetInstance();
		HRESULT result = terminalApi.triggerResize(m_terminal, width, height, &dimensions);

		// TerminalTriggerResize sizes the renderer and also moves the terminal HWND to (0, 0).
		// Position it within the Explorer++ client area after that internal resize.
		SetWindowPos(m_window, nullptr, x, y, width, height, flags);

		if (SUCCEEDED(result) && dimensions.x > 0 && dimensions.y > 0)
		{
			COORD pseudoConsoleSize{ static_cast<SHORT>(std::min(dimensions.x, SHRT_MAX)),
				static_cast<SHORT>(std::min(dimensions.y, SHRT_MAX)) };
			PseudoConsoleApi::GetInstance().resize(m_pseudoConsole, pseudoConsoleSize);
		}

		UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(m_window);

		if (dpi != m_dpi)
		{
			m_dpi = dpi;
			terminalApi.dpiChanged(m_terminal, dpi);
		}
	}

	void SetFontSize(int fontSize)
	{
		short terminalFontSize =
			static_cast<short>(std::clamp(fontSize, 1, static_cast<int>(SHRT_MAX)));

		if (terminalFontSize == m_fontSize)
		{
			return;
		}

		m_fontSize = terminalFontSize;
		TerminalControlApi::GetInstance().setTheme(m_terminal, DEFAULT_TERMINAL_THEME,
			TERMINAL_FONT_FAMILY.data(), terminalFontSize, m_dpi);

		if (!m_pseudoConsole)
		{
			return;
		}

		RECT bounds{};
		GetWindowRect(m_window, &bounds);
		MapWindowPoints(nullptr, GetParent(m_window), reinterpret_cast<POINT *>(&bounds), 2);
		SetBounds(bounds.left, bounds.top, bounds.right - bounds.left, bounds.bottom - bounds.top,
			IsWindowVisible(m_window));
	}

	void Focus()
	{
		SetActive();
		SetFocus(m_window);
	}

	void SetDirectoryChangedCallback(
		std::function<void(const std::wstring &)> directoryChangedCallback)
	{
		m_directoryChangedCallback = std::move(directoryChangedCallback);
	}

private:
	Impl() = default;

	bool StartPseudoConsole(const std::wstring &directory)
	{
		SECURITY_ATTRIBUTES securityAttributes{ sizeof(securityAttributes), nullptr, TRUE };
		HANDLE pseudoConsoleInputRead = nullptr;
		HANDLE hostInputWrite = nullptr;
		HANDLE hostOutputRead = nullptr;
		HANDLE pseudoConsoleOutputWrite = nullptr;

		if (!CreatePipe(&pseudoConsoleInputRead, &hostInputWrite, &securityAttributes, 0))
		{
			return false;
		}

		wil::unique_handle inputRead(pseudoConsoleInputRead);
		m_inputWrite.reset(hostInputWrite);
		SetHandleInformation(m_inputWrite.get(), HANDLE_FLAG_INHERIT, 0);

		if (!CreatePipe(&hostOutputRead, &pseudoConsoleOutputWrite, &securityAttributes, 0))
		{
			return false;
		}

		m_outputRead.reset(hostOutputRead);
		wil::unique_handle outputWrite(pseudoConsoleOutputWrite);
		SetHandleInformation(m_outputRead.get(), HANDLE_FLAG_INHERIT, 0);

		HRESULT result = PseudoConsoleApi::GetInstance().create(DEFAULT_TERMINAL_SIZE,
			inputRead.get(), outputWrite.get(), 0, &m_pseudoConsole);

		if (FAILED(result))
		{
			LOG(WARNING) << "CreatePseudoConsole failed: " << std::hex << result;
			return false;
		}

		inputRead.reset();
		outputWrite.reset();

		SIZE_T attributeListSize = 0;
		InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeListSize);
		auto attributeListStorage = std::make_unique<std::byte[]>(attributeListSize);
		auto *attributeList =
			reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(attributeListStorage.get());

		if (!InitializeProcThreadAttributeList(attributeList, 1, 0, &attributeListSize))
		{
			return false;
		}

		auto deleteAttributeList =
			wil::scope_exit([attributeList] { DeleteProcThreadAttributeList(attributeList); });

		if (!UpdateProcThreadAttribute(attributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE_VALUE,
				m_pseudoConsole, sizeof(m_pseudoConsole), nullptr, nullptr))
		{
			return false;
		}

		wchar_t systemDirectory[MAX_PATH];
		UINT systemDirectoryLength =
			GetSystemDirectory(systemDirectory, std::size(systemDirectory));

		if (systemDirectoryLength == 0 || systemDirectoryLength >= std::size(systemDirectory))
		{
			return false;
		}

		std::filesystem::path cmdPath(systemDirectory);
		cmdPath /= L"cmd.exe";

		std::wstring commandLine = QuoteArgument(cmdPath.wstring()) + L" /K ";
		std::wstring processDirectory = directory;

		if (PathIsUNC(directory.c_str()))
		{
			commandLine += L"pushd " + QuoteArgument(directory) + L" & ";
			processDirectory = systemDirectory;
		}

		// OSC 9;9 is the Windows Terminal working-directory sequence. Emitting it as part of every
		// prompt lets the tab title follow cd commands without polling or inspecting the process.
		commandLine += L"prompt $E]9;9;$P$E\\$P$G";

		STARTUPINFOEX startupInfo{};
		startupInfo.StartupInfo.cb = sizeof(startupInfo);
		startupInfo.lpAttributeList = attributeList;
		PROCESS_INFORMATION processInformation{};

		BOOL created = CreateProcess(cmdPath.c_str(), commandLine.data(), nullptr, nullptr, FALSE,
			EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT, nullptr,
			processDirectory.c_str(), &startupInfo.StartupInfo, &processInformation);

		if (!created)
		{
			LOG(WARNING) << "Could not start cmd.exe for the embedded terminal: " << GetLastError();
			return false;
		}

		m_process.reset(processInformation.hProcess);
		CloseHandle(processInformation.hThread);
		return true;
	}

	void ReadOutput()
	{
		std::string pending;
		char buffer[8192];

		while (!m_stopping)
		{
			DWORD bytesRead = 0;

			if (!ReadFile(m_outputRead.get(), buffer, sizeof(buffer), &bytesRead, nullptr)
				|| bytesRead == 0)
			{
				break;
			}

			pending.append(buffer, bytesRead);
			FlushUtf8(pending, false);
		}

		FlushUtf8(pending, true);
	}

	void FlushUtf8(std::string &pending, bool final)
	{
		if (pending.empty() || m_stopping)
		{
			return;
		}

		size_t maximumTail = final ? 0 : std::min<size_t>(3, pending.size());

		for (size_t tail = 0; tail <= maximumTail; ++tail)
		{
			size_t prefixSize = pending.size() - tail;

			if (prefixSize == 0)
			{
				return;
			}

			int requiredSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pending.data(),
				static_cast<int>(prefixSize), nullptr, 0);

			if (requiredSize == 0)
			{
				continue;
			}

			std::wstring output(requiredSize, L'\0');
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pending.data(),
				static_cast<int>(prefixSize), output.data(), requiredSize);
			InspectOutput(output);
			TerminalControlApi::GetInstance().sendOutput(m_terminal, output.c_str());
			pending.erase(0, prefixSize);
			return;
		}

		// If the stream contains malformed UTF-8, emit replacement characters instead of
		// permanently blocking all subsequent output.
		int requiredSize = MultiByteToWideChar(CP_UTF8, 0, pending.data(),
			static_cast<int>(pending.size()), nullptr, 0);

		if (requiredSize > 0)
		{
			std::wstring output(requiredSize, L'\0');
			MultiByteToWideChar(CP_UTF8, 0, pending.data(), static_cast<int>(pending.size()),
				output.data(), requiredSize);
			InspectOutput(output);
			TerminalControlApi::GetInstance().sendOutput(m_terminal, output.c_str());
		}

		pending.clear();
	}

	void InspectOutput(std::wstring_view output)
	{
		m_outputForDirectoryDetection.append(output);

		while (true)
		{
			auto sequenceStart =
				m_outputForDirectoryDetection.find(CURRENT_DIRECTORY_SEQUENCE_PREFIX);

			if (sequenceStart == std::wstring::npos)
			{
				size_t retainedLength = std::min(m_outputForDirectoryDetection.size(),
					CURRENT_DIRECTORY_SEQUENCE_PREFIX.size() - 1);
				m_outputForDirectoryDetection.erase(0,
					m_outputForDirectoryDetection.size() - retainedLength);
				return;
			}

			if (sequenceStart > 0)
			{
				m_outputForDirectoryDetection.erase(0, sequenceStart);
			}

			size_t valueStart = CURRENT_DIRECTORY_SEQUENCE_PREFIX.size();
			size_t stringTerminator = m_outputForDirectoryDetection.find(L"\x1b\\", valueStart);
			size_t bellTerminator = m_outputForDirectoryDetection.find(L'\a', valueStart);
			size_t sequenceEnd = std::min(stringTerminator, bellTerminator);

			if (sequenceEnd == std::wstring::npos)
			{
				if (m_outputForDirectoryDetection.size() > 32768)
				{
					m_outputForDirectoryDetection.clear();
				}

				return;
			}

			QueueDirectoryChanged(
				m_outputForDirectoryDetection.substr(valueStart, sequenceEnd - valueStart));

			size_t terminatorLength = sequenceEnd == stringTerminator ? 2 : 1;
			m_outputForDirectoryDetection.erase(0, sequenceEnd + terminatorLength);
		}
	}

	void QueueDirectoryChanged(const std::wstring &directory)
	{
		bool shouldPostMessage = false;

		{
			std::scoped_lock lock(m_directoryMutex);
			m_pendingDirectory = directory;

			if (!m_directoryNotificationPending)
			{
				m_directoryNotificationPending = true;
				shouldPostMessage = true;
			}
		}

		if (shouldPostMessage && !PostMessage(m_window, TERMINAL_DIRECTORY_CHANGED_MESSAGE, 0, 0))
		{
			std::scoped_lock lock(m_directoryMutex);
			m_directoryNotificationPending = false;
		}
	}

	void NotifyDirectoryChanged()
	{
		std::optional<std::wstring> directory;

		{
			std::scoped_lock lock(m_directoryMutex);
			directory = std::move(m_pendingDirectory);
			m_pendingDirectory.reset();
			m_directoryNotificationPending = false;
		}

		if (directory && m_directoryChangedCallback)
		{
			m_directoryChangedCallback(*directory);
		}
	}

	static void __stdcall OnTerminalWrite(wchar_t *data)
	{
		std::wstring input;

		if (data)
		{
			input = data;
			CoTaskMemFree(data);
		}

		std::scoped_lock lock(s_activeHostMutex);

		if (s_activeHost)
		{
			s_activeHost->WriteInput(input);
		}
	}

	void WriteInput(const std::wstring &input)
	{
		auto utf8 = ToUtf8(input);

		if (utf8.empty() || !m_inputWrite)
		{
			return;
		}

		DWORD bytesWritten = 0;
		WriteFile(m_inputWrite.get(), utf8.data(), static_cast<DWORD>(utf8.size()), &bytesWritten,
			nullptr);
	}

	void SetActive()
	{
		std::scoped_lock lock(s_activeHostMutex);
		s_activeHost = this;
	}

	LRESULT WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == TERMINAL_DIRECTORY_CHANGED_MESSAGE)
		{
			NotifyDirectoryChanged();
			return 0;
		}

		auto &terminalApi = TerminalControlApi::GetInstance();
		unsigned short scanCode = static_cast<unsigned short>((lParam >> 16) & 0xff);
		unsigned short flags = static_cast<unsigned short>((lParam >> 16) & 0xff00);

		switch (message)
		{
		case WM_MOUSEACTIVATE:
			SetActive();
			SetFocus(hwnd);
			break;

		case WM_SETFOCUS:
			SetActive();
			terminalApi.setFocus(m_terminal);
			break;

		case WM_KILLFOCUS:
			terminalApi.killFocus(m_terminal);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			terminalApi.sendKeyEvent(m_terminal, static_cast<unsigned short>(wParam), scanCode,
				flags, true);
			return 0;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			terminalApi.sendKeyEvent(m_terminal, static_cast<unsigned short>(wParam), scanCode,
				flags, false);
			return 0;

		case WM_CHAR:
		case WM_SYSCHAR:
			terminalApi.sendCharEvent(m_terminal, static_cast<wchar_t>(wParam), scanCode, flags);
			return 0;
		}

		return DefSubclassProc(hwnd, message, wParam, lParam);
	}

	static inline std::mutex s_activeHostMutex;
	static inline Impl *s_activeHost = nullptr;

	HWND m_window = nullptr;
	void *m_terminal = nullptr;
	HANDLE m_pseudoConsole = nullptr;
	wil::unique_handle m_inputWrite;
	wil::unique_handle m_outputRead;
	wil::unique_process_handle m_process;
	std::unique_ptr<WindowSubclass> m_windowSubclass;
	std::thread m_outputThread;
	std::atomic_bool m_stopping = false;
	UINT m_dpi = 0;
	short m_fontSize = 0;
	std::wstring m_outputForDirectoryDetection;
	std::mutex m_directoryMutex;
	std::optional<std::wstring> m_pendingDirectory;
	bool m_directoryNotificationPending = false;
	std::function<void(const std::wstring &)> m_directoryChangedCallback;
};

std::unique_ptr<TerminalHost> TerminalHost::Create(HWND parent, const std::wstring &directory)
{
	auto impl = Impl::Create(parent, directory);

	if (!impl)
	{
		return nullptr;
	}

	return std::unique_ptr<TerminalHost>(new TerminalHost(std::move(impl)));
}

TerminalHost::TerminalHost(std::unique_ptr<Impl> impl) : m_impl(std::move(impl))
{
}

TerminalHost::~TerminalHost() = default;

HWND TerminalHost::GetHWND() const
{
	return m_impl->GetHWND();
}

void TerminalHost::SetBounds(int x, int y, int width, int height, bool visible)
{
	m_impl->SetBounds(x, y, width, height, visible);
}

void TerminalHost::SetFontSize(int fontSize)
{
	m_impl->SetFontSize(fontSize);
}

void TerminalHost::Focus()
{
	m_impl->Focus();
}

void TerminalHost::SetDirectoryChangedCallback(
	std::function<void(const std::wstring &)> directoryChangedCallback)
{
	m_impl->SetDirectoryChangedCallback(std::move(directoryChangedCallback));
}
