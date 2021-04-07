#include "control.h"

int _tmain(int argc, TCHAR **argv, TCHAR **envp) {
	// Check process instance
	// Close process if another instance is running

	Config *cfg = calloc(1, sizeof(Config));
	if (cfg == NULL)
		return -1;

	if (!init_config(cfg)) {
		return -1;
	}

	end_config(cfg);
	free(cfg);
	cfg = NULL;
	return 0;
}