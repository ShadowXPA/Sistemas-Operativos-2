#ifdef __cplusplus
extern "C" {
#endif

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
#include <string.h>

	DLL_API void clear_input_stream(const FILE *const);

#define _sout(p, x, ...) _ftprintf_s(p, _T(x), __VA_ARGS__)
#define sout(x, ...) _sout(stdout, x, __VA_ARGS__)
#define _sin(p, x, ...) {\
							_ftscanf_s(p, _T(x), __VA_ARGS__);\
							clear_input_stream(p);\
						}
#define sin(x, ...) _sin(stdin, x, __VA_ARGS__)
#define _cmp(str1, str2) _tcscmp(str1, str2)
#define cmp(str1, str2) _cmp(str1, _T(str2))
#define _icmp(str1, str2) _tcsicmp(str1, str2)
#define icmp(str1, str2) _tcsicmp(str1, _T(str2))
#define _contains(str, search) _tcsstr(str, search)
#define contains(str, search) _contains(str, _T(search))
#define _cpy(dest, src, size) _tcscpy_s(dest, size, src)
#define cpy(dest, src, size) _cpy(dest, src, size)

#define MAX_MAP 1000
#define MAX_NAME 50
#define MAX_SHARED_BUFFER 20

#define MAX_AIRPORT 90
#define MAX_AIRPLANE 100
#define MAX_PASSENGER (100 * MAX_AIRPLANE)

#define DEFAULT_CIN_BUFFER "%49[^\n]"

#define MTX_MEMORY _T("MTXSharedMemory")
#define FILE_MAPPING_NAME _T("ControlAviao")
#define STOP_SYSTEM_EVENT _T("StopEvent")

	DLL_API typedef struct point {
		unsigned int x, y;
	} Point;

	DLL_API typedef struct airport {
		unsigned int id;					// 1 ~ 90
		unsigned int active : 1;
		TCHAR name[MAX_NAME];
		Point coordinates;					// 0 ~ 1000
	} Airport;

	DLL_API typedef struct airplane {
		unsigned int id;					// 91 ~ 190, MAYBE ADD PID?
		unsigned int active : 1;
		TCHAR name[MAX_NAME];
		int velocity;
		int capacity;						// Current capacity
		int max_capacity;					// Maximum capacity
		Point coordinates;					// 0 ~ 1000
		unsigned int airport_start;
		unsigned int airport_end;
	} Airplane;

	DLL_API typedef struct passenger {
		unsigned int id;					// 191 ~ 1190
		unsigned int active : 1;
		TCHAR name[MAX_NAME];
		int wait_time;
		unsigned int airport;
		unsigned int airport_end;
		unsigned int airplane;				// 0 not in airplane, 91 ~ 190 in an airplane
	} Passenger;

	DLL_API typedef union command {
		Point coordinates;
		TCHAR str[MAX_NAME];
	} Command;

	DLL_API typedef struct sharedbuffer {
		unsigned int id;					//TODO Maybe change to PID...
		unsigned int cmd_id;
		Command command;
	} SharedBuffer;

	DLL_API typedef struct sharedmemory {
		unsigned int map[MAX_MAP][MAX_MAP];	// IDs (0 = empty, 1 ~ 90 = airports, 91 ~ 190 = airplanes)
		BOOL accepting_planes;
		int inC, outC;
		SharedBuffer bufferControl[MAX_SHARED_BUFFER];
		int inA, outA;
		SharedBuffer bufferAirplane[MAX_SHARED_BUFFER];
	} SharedMemory;

	/*DLL_API BOOL createOrOpenRegistry(const TCHAR *subkey, HKEY *key, DWORD *result);
	DLL_API BOOL createOrEditRegistryValue(const TCHAR *subkey, const TCHAR *subvalue, DWORD dwType, HKEY *key);
	DLL_API BOOL viewRegistryValue(const TCHAR *subkey, const TCHAR *subvalue, DWORD *cbData, HKEY *key);
	DLL_API BOOL deleteRegistryValue(const TCHAR *subkey, HKEY *key);

	DLL_API BOOL createThread(HANDLE *h, LPTHREAD_START_ROUTINE routine, void *data, DWORD flags, DWORD *threadId);*/

#endif // !UTILS_H

#ifdef __cplusplus
}
#endif
