#include "aviao.h"

BOOL init_config(Config* cfg) {
	memset(cfg, 0, sizeof(Config));

	cfg->mtx_memory = OpenMutex(NULL, FALSE, MTX_MEMORY);
	if (cfg->mtx_memory == NULL)
		return FALSE;

	cfg->stop_event = OpenEvent(NULL, FALSE, STOP_SYSTEM_EVENT);
	if (cfg->stop_event == NULL)
		return FALSE;

	return TRUE;
}

void end_config(Config* cfg) {
	UnmapViewOfFile(cfg->memory);
	CloseHandle(cfg->obj_map);
	CloseHandle(cfg->mtx_memory);
	CloseHandle(cfg->stop_event);
}

void init_aviao(Config* cfg) {
	TCHAR buffer[MAX_NAME] = { 0 };
	// init window (Win32)

	// init shared memory
	cfg->obj_map = OpenFileMapping(NULL, FALSE, FILE_MAPPING_NAME);
	if (cfg->obj_map == NULL)
		return;
	cfg->memory = (SharedMemory*)MapViewOfFile(cfg->obj_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
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

DWORD WINAPI read_command(void* param) {
	Config* cfg = (Config*)param;
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
		}
		else if (icmp(buffer, "board") == 0) {
			//send confirmation to controler
			//receive number of passengers
		}
		else if (icmp(buffer, "start") == 0) {
			//send confirmation to controler that I want to start the trip
			//receive OK from controler
		}
		else if (icmp(buffer, "help") == 0) {
			// show all commands
			sout("help   -> Shows this\n");
			sout("destination -> Define trip destination\n");
			sout("board -> Board passengers in the airplane\n");
			sout("start -> Start the trip\n");
			sout("exit   -> Stops the whole system\n");
		}
		else if (icmp(buffer, "exit") == 0) {
			sout("Stopping system...\n");
		}
		else {
			sout("Invalid command!\n");
		}
	} while (icmp(buffer, "exit") != 0);
	//event to finish...
	//SetEvent(cfg->stop_event);
	return 0;
}

DWORD WINAPI read_shared_memory(void* param) {

}

DWORD WINAPI send_heartbeat(void* param)
{

	return 0;
}

