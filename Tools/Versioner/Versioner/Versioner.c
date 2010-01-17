/******************************************************************
 *
 * Project: Versioner
 * File: Versioner.c
 * License: GPL - See COPYING in the top level directory
 *
 * Writes the current date/time to the specified
 * input file.
 *
 * Written by David Erceg
 * www.explorerplusplus.com
 *
 *****************************************************************/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>


/* Disable 'unreferenced formal parameter' warning. */
#pragma warning( disable : 4100 )

#define SIZEOF_ARRAY(array)	sizeof(array) / sizeof(array[0])

#define STRING_DATE "VERSION_BUILD_DATE"

int main(int argc,char **argv)
{
	HANDLE hFile;
	SYSTEMTIME st;
	DWORD nBytesWritten;
	char szOutput[64];
	char szDate[64];

	GetLocalTime(&st);

	sprintf_s(szDate,SIZEOF_ARRAY(szDate),"%02d/%02d/%02d %02d:%02d:%02d",st.wDay,st.wMonth,st.wYear,
	st.wHour,st.wMinute,st.wSecond);

	sprintf_s(szOutput,SIZEOF_ARRAY(szOutput),"#define " STRING_DATE	"\t_T(\"%s\")"	"\n",szDate);

	puts(szOutput);

	hFile = CreateFile(argv[1],GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
	0,NULL);

	SetFilePointer(hFile,0,NULL,FILE_BEGIN);
	WriteFile(hFile,szOutput,lstrlen(szOutput),&nBytesWritten,NULL);

	CloseHandle(hFile);
}