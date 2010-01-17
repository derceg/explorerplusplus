/******************************************************************
 *
 * Project: ShellBrowser
 * File: iPathManager.cpp
 *
 * Remembers path history, and
 * includes the ability to browse
 * back/forward through a set
 * of paths.
 *
 * Written by David Erceg
 *
 *****************************************************************/

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES	1

#include "stdafx.h"
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <objidl.h>
#include "IShellView.h"
#include "iShellBrowser_internal.h"