#include <fcntl.h>
#include "../Utils/utils.h"

typedef struct cfg {
	int (*move) (int, int, int, int, int*, int*);
	Airplane airplane;
	SharedMemory* memory;				// MapViewOfFile
	// flying
	BOOL flying;
	// Handles
	HANDLE obj_map;
	HANDLE mtx_memory;					// Mutex for shared memory access
	HANDLE stop_event;					// Kill event (Stops the whole system)
} Config;

int init_config(Config*);
void end_config(Config* cfg);
void init_aviao(Config* cfg);

DWORD WINAPI read_command(void* param);
DWORD WINAPI read_shared_memory(void* param);
DWORD WINAPI send_heartbeat(void* param);