#include <fcntl.h>
#include "../Utils/utils.h"

#define MTX_C _T("AviaoMutexC")
#define MTX_A _T("AviaoMutexA")

typedef struct cfg {
	BOOL die;
	int (*move) (int, int, int, int, int *, int *);
	Airplane airplane;
	SharedMemory *memory;				// MapViewOfFile
	// Plane is flying
	BOOL flying;
	// Handles
	HANDLE obj_map;
	HANDLE mtx_memory;					// Mutex for shared memory access
	HANDLE stop_event;					// Kill event (Stops the whole system)
	HANDLE stop_airplane;				// Kill airplane
	/*
		Semaphores and circular buffer handlers
	*/
	HANDLE sem_emptyC;
	HANDLE sem_itemC;
	HANDLE mtx_C;
	HANDLE sem_emptyA;
	HANDLE sem_itemA;
	HANDLE mtx_A;
} Config;

int init_config(Config *);
void end_config(Config *cfg);
void init_aviao(Config *cfg);

void receive_command(Config *, SharedBuffer *);
void send_command(Config *, SharedBuffer *);

DWORD WINAPI read_command(void *param);
DWORD WINAPI read_shared_memory(void *param);
DWORD WINAPI send_heartbeat(void *param);
