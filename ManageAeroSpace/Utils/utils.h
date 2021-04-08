#ifndef UTILS_H
#define UTILS_H

#ifdef _WINDLL
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>

DLL_API void clearInputStream(const FILE *const);

#define _sout(p, x, ...) _ftprintf_s(p, _T(x), __VA_ARGS__)
#define sout(x, ...) _sout(stdout, x, __VA_ARGS__)
#define _sin(p, x, ...) {\
							_ftscanf_s(p, _T(x), __VA_ARGS__);\
							clearInputStream(p);\
						}
#define sin(x, ...) _sin(stdin, x, __VA_ARGS__)

DLL_API BOOL createOrOpenRegistry(const TCHAR *subkey, HKEY *key, DWORD *result);
DLL_API BOOL createOrEditRegistryValue(const TCHAR *subkey, const TCHAR *subvalue, DWORD dwType, HKEY *key);
DLL_API BOOL viewRegistryValue(const TCHAR *subkey, const TCHAR *subvalue, DWORD *cbData, HKEY *key);
DLL_API BOOL deleteRegistryValue(const TCHAR *subkey, HKEY *key);

DLL_API BOOL createThread(HANDLE *h, LPTHREAD_START_ROUTINE routine, void *data, DWORD flags, DWORD *threadId);

#endif // !UTILS_H