// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

/* Used to share global data across the
classes within the Explorer++ project. */
namespace NExplorerplusplus
{
const TCHAR APP_NAME[] = _T("Explorer++");
const TCHAR CLASS_NAME[] = _T("Explorer++");

constexpr WCHAR DOCUMENTATION_LINK[] = L"https://explorerplusplus.readthedocs.io/en/latest/";

/* Registry keys used to store program settings. */
const TCHAR REG_MAIN_KEY[] = _T("Software\\Explorer++");
const TCHAR REG_SETTINGS_KEY[] = _T("Software\\Explorer++\\Settings");

/* The name of the XML file that preferences are
saved to/loaded from. */
const TCHAR XML_FILENAME[] = _T("config.xml");

// Internal command line arguments.
const TCHAR JUMPLIST_TASK_NEWTAB_ARGUMENT[] = _T("--open-new-tab");
const TCHAR APPLICATION_CRASHED_ARGUMENT[] = _T("--application-crashed");

const TCHAR LANGUAGE_DLL_FILENAME_PATTERN[] = _T("Explorer++*.dll");

// The first instance of the application will create a mutex with this name, which later instances
// can then use to detect whether or not the application is already running.
const TCHAR APPLICATION_MUTEX_NAME[] = _T("Explorer++");
}

/* Used when setting Explorer++ as the default
file manager. */
static const TCHAR SHELL_DEFAULT_INTERNAL_COMMAND_NAME[] = _T("openinexplorer++");

#define WM_APP_ASSOCCHANGED (WM_APP + 54)

/* Rebar menu id's. */
#define ID_REBAR_MENU_BACK_START 2000
#define ID_REBAR_MENU_BACK_END 2999
#define ID_REBAR_MENU_FORWARD_START 3000
#define ID_REBAR_MENU_FORWARD_END 3999

enum ExitCode
{
	EXIT_CODE_NORMAL,
	EXIT_CODE_NORMAL_EXISTING_PROCESS,
	EXIT_CODE_NORMAL_CRASH_HANDLER,
	EXIT_CODE_ERROR
};

extern HACCEL g_hAccl;

BOOL TestConfigFileInternal(void);
