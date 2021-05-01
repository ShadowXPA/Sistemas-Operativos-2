#include "control.h"
#include <fcntl.h>

#define _sout(p, x, ...) _ftprintf_s(p, _T(x), __VA_ARGS__)
#define sout(x, ...) _sout(stdout, x, __VA_ARGS__)
#define MTX_CTR _T("Control")

void exit_control(Config *);

int _tmain(int argc, TCHAR **argv, TCHAR **envp) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	HANDLE mutex;
	//mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, MTX_CTR);
	//if (mutex == NULL) {
	//	mutex = CreateMutex(NULL, TRUE, MTX_CTR);
	//} else {
	//	sout("Application already running\n");
	//	return -1;
	//}

	mutex = CreateMutex(NULL, TRUE, MTX_CTR);
	DWORD already_exists = GetLastError();
	if (already_exists == ERROR_ALREADY_EXISTS) {
		sout("Application already running\n");
		return -1;
	}

	Config *cfg = malloc(sizeof(Config));
	if (cfg == NULL)
		return -1;

	if (!init_config(cfg)) {
		exit_control(cfg);
		return -1;
	}

	exit_control(cfg);
	if (mutex != NULL)
		ReleaseMutex(mutex);
	return 0;
}

void exit_control(Config *cfg) {
	end_config(cfg);
	free(cfg);
	cfg = NULL;
}
