// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>



// TODO: reference additional headers your program requires here
#include <windows.h>
#include <stdlib.h>

extern "C" {
	char *getenv(const char *varname);
	WINBASEAPI HANDLE WINAPI CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	WINBASEAPI HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
	WINPATHCCHAPI HRESULT APIENTRY PathCchRemoveFileSpec(PWSTR pszPath, size_t cchPath);
}

extern BOOL verbose;