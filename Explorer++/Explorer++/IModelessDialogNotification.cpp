// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IModelessDialogNotification.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "ModelessDialogs.h"


CModelessDialogNotification::CModelessDialogNotification() :
m_RefCount(1)
{

}

ULONG CModelessDialogNotification::AddRef(void)
{
	return ++m_RefCount;
}

ULONG CModelessDialogNotification::Release(void)
{
	m_RefCount--;
	
	if(m_RefCount == 0)
	{
		delete this;
		return 0;
	}

	return m_RefCount;
}

void CModelessDialogNotification::OnModelessDialogDestroy(int iResource)
{
	switch(iResource)
	{
	case IDD_SEARCH:
		g_hwndSearch = NULL;
		break;

	case IDD_SCRIPTING:
		g_hwndRunScript = NULL;
		break;

	case IDD_MANAGE_BOOKMARKS:
		g_hwndManageBookmarks = NULL;
		break;
	}
}