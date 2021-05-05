#include "aviao.h"

void exit_aviao(Config* cfg);

int _tmain(int argc, TCHAR** argv, TCHAR** envp) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	_setmode(_fileno(stderr), _O_WTEXT);
#endif
	//Read command line args
	//verify args

	//criar a config para o aviao
	Config* cfg = malloc(sizeof(Config));
	if (cfg == NULL)
		return -1;

	if (!init_config(cfg)) {
		exit_aviao(cfg);
		return -1;
	}

	if (argc != 4) {
		exit_aviao(cfg);
		return -1;
	}
	cfg->airplane.max_capacity = _ttoi(argv[1]);
	cfg->airplane.velocity = _ttoi(argv[2]);
	cfg->airplane.airport_start = _ttoi(argv[3]);

	//TODO remove this shet
	cfg->airplane.pid = GetProcessId(GetCurrentProcess());

	init_aviao(cfg);

	exit_aviao(cfg);
	return 0;
}

void exit_aviao(Config* cfg) {
	end_config(cfg);
	free(cfg);
	cfg = NULL;
}
