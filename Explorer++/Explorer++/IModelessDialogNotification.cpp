// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IModelessDialogNotification.h"
#include "MainResource.h"
#include "ModelessDialogs.h"


ModelessDialogNotification::ModelessDialogNotification() :
m_RefCount(1)
{

}

ULONG ModelessDialogNotification::AddRef(void)
{
	return ++m_RefCount;
}

ULONG ModelessDialogNotification::Release(void)
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

void ModelessDialogNotification::OnModelessDialogDestroy(int iResource)
{
	switch(iResource)
	{
	case IDD_SEARCH:
		g_hwndSearch = nullptr;
		break;

	case IDD_SCRIPTING:
		g_hwndRunScript = nullptr;
		break;

	case IDD_MANAGE_BOOKMARKS:
		g_hwndManageBookmarks = nullptr;
		break;
	}
}