/******************************************************************
 *
 * Project: Explorer++
 * File: IModelessDialogNotification.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Handles modeless dialog notifications.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "Explorer++_internal.h"
#include "IModelessDialogNotification.h"
#include "MainResource.h"


CModelessDialogNotification::CModelessDialogNotification() :
m_RefCount(1)
{

}

CModelessDialogNotification::~CModelessDialogNotification()
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

	case IDD_MANAGE_BOOKMARKS:
		g_hwndManageBookmarks = NULL;
		break;
	}
}