#include "aviao.h"

BOOL init_config(Config *cfg) {
	memset(cfg, 0, sizeof(Config));

	cfg->mtx_memory = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MTX_MEMORY);
	if (cfg->mtx_memory == NULL)
		return FALSE;

	cfg->sem_emptyC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_EMPTY_C);
	cfg->sem_itemC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_ITEM_C);
	cfg->sem_emptyA = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_EMPTY_A);
	cfg->sem_itemA = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_ITEM_A);
	if (cfg->sem_emptyC == NULL || cfg->sem_emptyA == NULL || cfg->sem_itemC == NULL || cfg->sem_itemA == NULL)
		return FALSE;

	cfg->mtx_C = CreateMutex(NULL, FALSE, MTX_C);
	cfg->mtx_A = CreateMutex(NULL, FALSE, MTX_A);
	if (cfg->mtx_C == NULL || cfg->mtx_A == NULL)
		return FALSE;

	cfg->stop_event = OpenEvent(EVENT_ALL_ACCESS, FALSE, STOP_SYSTEM_EVENT);
	if (cfg->stop_event == NULL)
		return FALSE;

	cfg->stop_airplane = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (cfg->stop_airplane == NULL)
		return FALSE;

	return TRUE;
}

void end_config(Config *cfg) {
	UnmapViewOfFile(cfg->memory);
	CloseHandle(cfg->obj_map);
	CloseHandle(cfg->mtx_memory);
	CloseHandle(cfg->sem_emptyC);
	CloseHandle(cfg->sem_emptyA);
	CloseHandle(cfg->sem_itemC);
	CloseHandle(cfg->sem_itemA);
	CloseHandle(cfg->mtx_C);
	CloseHandle(cfg->mtx_A);
	CloseHandle(cfg->stop_event);
	CloseHandle(cfg->stop_airplane);
}

void init_aviao(Config *cfg) {
	TCHAR buffer[MAX_NAME] = { 0 };
	// init window (Win32)

	// init shared memory
	cfg->obj_map = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FILE_MAPPING_NAME);
	if (cfg->obj_map == NULL)
		return;
	cfg->memory = (SharedMemory *) MapViewOfFile(cfg->obj_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (cfg->memory == NULL)
		return;

	//name of the plane
	sout("Input airplane name:\n > ");
	sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
	_cpy(cfg->airplane.name, buffer, MAX_NAME);

	//send hello!, name, capacity, velocity and initial airport

	//receive coordinates

	// init threads:
	HANDLE thread[3];
	// read command
	thread[0] = CreateThread(NULL, 0, read_command, cfg, 0, NULL);
	if (thread[0] == NULL)
		return;
	thread[1] = CreateThread(NULL, 0, read_shared_memory, cfg, 0, NULL);
	if (thread[1] == NULL)
		return;
	thread[2] = CreateThread(NULL, 0, send_heartbeat, cfg, 0, NULL);
	if (thread[2] == NULL)
		return;

	WaitForMultipleObjects(3, thread, TRUE, INFINITE);

	CloseHandle(thread[0]);
	CloseHandle(thread[1]);
	CloseHandle(thread[2]);
}

DWORD WINAPI read_command(void *param) {
	Config *cfg = (Config *) param;
	TCHAR buffer[MAX_NAME] = { 0 };
	do {
		sout("Input command:\n > ");
		sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
		//define destination
		//board passengers
		//start trip
		//exit

		if (icmp(buffer, "destination") == 0) {
			// define destination
			Airport airport;
			sout("Input airport name:\n > ");
			sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			_cpy(airport.name, buffer, MAX_NAME);
			//send airport to control
			//receive int
			//add airport to destination
		} else if (icmp(buffer, "board") == 0) {
			//send confirmation to controler
			//receive number of passengers
		} else if (icmp(buffer, "start") == 0) {
			//send confirmation to controler that I want to start the trip
			//receive OK from controler
		} else if (icmp(buffer, "help") == 0) {
			// show all commands
			sout("help        -> Shows this\n");
			sout("destination -> Define trip destination\n");
			sout("board       -> Board passengers in the airplane\n");
			sout("start       -> Start the trip\n");
			sout("exit        -> Stops the airplane\n");
		} else if (icmp(buffer, "exit") == 0) {
			cfg->die = TRUE;
			sout("Stopping airplane...\n");
		} else {
			sout("Invalid command!\n");
		}
	} while (!cfg->die);
	//event to finish...
	//SetEvent(cfg->stop_event);
	SetEvent(cfg->stop_airplane);
	return 0;
}

DWORD WINAPI read_shared_memory(void *param) {
	Config *cfg = (Config *) param;
	HANDLE handles[3];
	SharedBuffer buffer;
	handles[0] = cfg->stop_event;
	handles[1] = cfg->stop_airplane;
	handles[2] = cfg->sem_itemA;
	DWORD res;
	while ((!((res = WaitForMultipleObjects(3, handles, FALSE, INFINITE)) == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1)))
		&& !cfg->die) {
		receive_command(cfg, &buffer);
		if (buffer.to_id == cfg->airplane.pid) {
			// Handle command
			// ...
			sout("Command: %d\nFrom: %u\nTo: %u\n\n", buffer.cmd_id, buffer.from_id, buffer.to_id);
		}
	}

	cfg->die = TRUE;

	return 0;
}

DWORD WINAPI send_heartbeat(void *param) {
	Config *cfg = (Config *) param;
	HANDLE handles[3];
	SharedBuffer buffer;
	handles[0] = cfg->stop_event;
	handles[1] = cfg->stop_airplane;
	handles[2] = cfg->sem_emptyC;
	DWORD res;
	buffer.from_id = cfg->airplane.pid;
	buffer.cmd_id = CMD_HEARTBEAT;
	while ((!((res = WaitForMultipleObjects(3, handles, FALSE, INFINITE)) == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1)))
		&& !cfg->die) {
		send_command(cfg, &buffer);
		Sleep(3000);
	}

	return 0;
}

BOOL receive_command(Config *cfg, SharedBuffer *sb) {
	WaitForSingleObject(cfg->mtx_A, INFINITE);
	CopyMemory(sb, &(cfg->memory->bufferAirplane[cfg->memory->outA]), sizeof(SharedBuffer));
	if (sb->to_id == cfg->airplane.pid) {
		cfg->memory->outA = (cfg->memory->outA + 1) % MAX_SHARED_BUFFER;
	}
	ReleaseMutex(cfg->mtx_A);
	(sb->to_id == cfg->airplane.pid) ? ReleaseSemaphore(cfg->sem_emptyA, 1, NULL) : ReleaseSemaphore(cfg->sem_itemA, 1, NULL);

	return FALSE;
}

BOOL send_command(Config *cfg, SharedBuffer *sb) {
	WaitForSingleObject(cfg->mtx_C, INFINITE);
	CopyMemory(&(cfg->memory->bufferControl[cfg->memory->inC]), sb, sizeof(SharedBuffer));
	cfg->memory->inC = (cfg->memory->inC + 1) % MAX_SHARED_BUFFER;
	ReleaseMutex(cfg->mtx_C);
	ReleaseSemaphore(cfg->sem_itemC, 1, NULL);

	return FALSE;
}
