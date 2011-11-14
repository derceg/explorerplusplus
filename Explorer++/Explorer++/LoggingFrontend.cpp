/******************************************************************
 *
 * Project: Explorer++
 * File: LoggingFrontend.cpp
 * License: GPL - See COPYING in the top level directory
 *
 * Provides a logging frontend for Pantheios. By default,
 * this frontend will enable all logging in debug mode,
 * and disable all logging in release mode. This setting
 * can then be changed dynamically.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include "stdafx.h"
#include "LoggingFrontend.h"


namespace
{
	bool g_EnableLogging =
#ifdef _DEBUG
		true;
#else
		false;
#endif;
}

PANTHEIOS_CALL(int) pantheios_fe_init(void *reserved,void **ptoken)
{
    *ptoken = NULL;

    return 0;
}

PANTHEIOS_CALL(void) pantheios_fe_uninit(void *token)
{

}

PANTHEIOS_CALL(PAN_CHAR_T const *) pantheios_fe_getProcessIdentity(void *token)
{
	return L"Explorer++";
}

PANTHEIOS_CALL(int) pantheios_fe_isSeverityLogged(void *token,int severity,int backEndId)
{
	if(g_EnableLogging)
	{
		return 1;
	}

	return 0;
}

bool NLoggingFrontend::CheckLoggingEnabled()
{
	return g_EnableLogging;
}

void NLoggingFrontend::EnableLogging(bool Enable)
{
	g_EnableLogging = Enable;
}