#include "control.h"
#include <fcntl.h>

#define _sout(p, x, ...) _ftprintf_s(p, _T(x), __VA_ARGS__)
#define sout(x, ...) _sout(stdout, x, __VA_ARGS__)
#define MTX_CTR "Control"

int _tmain(int argc, TCHAR **argv, TCHAR **envp) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif

	HANDLE mutex;

	mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, _T(MTX_CTR));

	if (mutex == NULL) {
		mutex = CreateMutex(NULL, TRUE, _T(MTX_CTR));
	} else {
		sout("Application already running\n");
		return -1;
	}

	Config *cfg = calloc(1, sizeof(Config));
	if (cfg == NULL)
		return -1;

	if (!init_config(cfg)) {
		return -1;
	}

	Sleep(1000);

	end_config(cfg);
	free(cfg);
	cfg = NULL;
	return 0;
}