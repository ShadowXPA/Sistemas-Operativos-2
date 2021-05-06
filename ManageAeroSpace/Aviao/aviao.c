#include "aviao.h"

BOOL init_config(Config *cfg) {
	memset(cfg, 0, sizeof(Config));

	cfg->hdll = LoadLibrary(DLL_AVIAO);
	if (cfg->hdll == NULL)
		return FALSE;

	cfg->move = (int (*) (int, int, int, int, int *, int *)) GetProcAddress(cfg->hdll, DLL_FUNC_MOVE);
	if (cfg->move == NULL)
		return FALSE;

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
	FreeLibrary(cfg->hdll);
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
	SharedBuffer sb;
	// init window (Win32)

	// init shared memory
	cfg->obj_map = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FILE_MAPPING_NAME);
	if (cfg->obj_map == NULL)
		return;
	cfg->memory = (SharedMemory *) MapViewOfFile(cfg->obj_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (cfg->memory == NULL)
		return;

	WaitForSingleObject(cfg->mtx_memory, INFINITE);
	if (!cfg->memory->accepting_planes) {
		ReleaseMutex(cfg->mtx_memory);
		return;
	}
	ReleaseMutex(cfg->mtx_memory);

	//name of the plane
	sout("Input airplane name:\n > ");
	sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
	_cpy(cfg->airplane.name, buffer, MAX_NAME);

	//send hello!, name, capacity, velocity and initial airport
	sb.cmd_id = CMD_HELLO;
	sb.from_id = cfg->airplane.pid;
	sb.command.airplane = cfg->airplane;
	BOOL bool = send_command(cfg, &sb);
	if (!bool) {
		sout("The controller is down!");
		return;
	}
	//receive coordinates
	receive_command(cfg, &sb);
	if (sb.cmd_id == (CMD_HELLO | CMD_OK)) {
		cfg->airplane = sb.command.airplane;
		sout("Airplane registered!\n");
		sout("Name: '%s' (ID: %u, PID: %u)\nVelocity: %d\nCapacity: %d\nMax. Capacity: %d\nCoordinates: (x: %u, y: %u)\nDeparture: '%s' (ID: %u)\nDestination: '%s' (ID: %u)\n\n",
			cfg->airplane.name, cfg->airplane.id, cfg->airplane.pid, cfg->airplane.velocity, cfg->airplane.capacity, cfg->airplane.max_capacity,
			cfg->airplane.coordinates.x, cfg->airplane.coordinates.y, cfg->airplane.airport_start.name, cfg->airplane.airport_start.id, cfg->airplane.airport_end.name, cfg->airplane.airport_end.id);
	} else {
		sout("Airplane not registered!\nError: '%s'\n", sb.command.str);
		return;
	}
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
	SharedBuffer sb;
	do {
		sout("Input command:\n > ");
		sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
		//define destination
		//board passengers
		//start trip
		//exit
		if (icmp(buffer, "exit") == 0) {
			cfg->die = TRUE;
			sout("Stopping airplane...\n");
		}

		if (!cfg->flying) {
			if (icmp(buffer, "destination") == 0) {
				// define destination
				Airport airport;
				sout("Input airport name:\n > ");
				sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
				_cpy(airport.name, buffer, MAX_NAME);
				//send airport to control
				sb.from_id = cfg->airplane.pid;
				sb.cmd_id = CMD_SEND_DESTINATION;
				sb.command.airport = airport;
				send_command(cfg, &sb);
			} else if (icmp(buffer, "board") == 0) {
				//send confirmation to controler
				sb.from_id = cfg->airplane.pid;
				sb.cmd_id = CMD_BOARD;
				send_command(cfg, &sb);
			} else if (icmp(buffer, "start") == 0) {
				//send confirmation to controler that I want to start the trip
				sb.from_id = cfg->airplane.pid;
				sb.cmd_id = CMD_LIFT_OFF;
				send_command(cfg, &sb);
			} else if (icmp(buffer, "help") == 0) {
				// show all commands
				sout("help        -> Shows this\n");
				sout("destination -> Define trip destination\n");
				sout("board       -> Board passengers in the airplane\n");
				sout("start       -> Start the trip\n");
				sout("exit        -> Stops the airplane\n");
			} else if (icmp(buffer, "list") == 0) {
				sout("Name: '%s' (ID: %u, PID: %u)\nVelocity: %d\nCapacity: %d\nMax. Capacity: %d\nCoordinates: (x: %u, y: %u)\nDeparture: '%s' (ID: %u)\nDestination: '%s' (ID: %u)\n\n",
					cfg->airplane.name, cfg->airplane.id, cfg->airplane.pid, cfg->airplane.velocity, cfg->airplane.capacity, cfg->airplane.max_capacity,
					cfg->airplane.coordinates.x, cfg->airplane.coordinates.y, cfg->airplane.airport_start.name, cfg->airplane.airport_start.id, cfg->airplane.airport_end.name,
					cfg->airplane.airport_end.id);
			} else {
				sout("Invalid command!\n");
			}
		}

	} while (!cfg->die);
	//event to finish...
	//SetEvent(cfg->stop_event);
	SetEvent(cfg->stop_airplane);
	return 0;
}

DWORD WINAPI read_shared_memory(void *param) {
	Config *cfg = (Config *) param;
	SharedBuffer buffer;
	while (!cfg->die) {
		if (receive_command(cfg, &buffer)) {
			if (buffer.to_id == cfg->airplane.pid) {
				switch (buffer.cmd_id) {
					case (CMD_SEND_DESTINATION | CMD_OK):
					{
						//add airport to airport end (destination)
						cfg->airplane.airport_end = buffer.command.airport;
						Point p = cfg->airplane.airport_end.coordinates;
						sout("Destination airport has been set!\nCoordinates: (%u, %u)\n", p.x, p.y);
						break;
					}
					case (CMD_SEND_DESTINATION | CMD_ERROR):
					{
						//error in send destination
						sout("Error: '%s'\n", buffer.command.str);
						break;
					}
					case (CMD_BOARD | CMD_OK):
					{
						//TODO (cagadous right now) receive number of passengers
						cfg->airplane.capacity = buffer.command.number;
						sout("Number of passangers in the plane: %d", cfg->airplane.capacity);
						break;
					}
					case (CMD_BOARD | CMD_ERROR):
					{
						//error in board passengers
						sout("Error: '%s'\n", buffer.command.str);
						break;
					}
					case (CMD_LIFT_OFF | CMD_OK):
					{
						//receive OK from controler that I can start the trip
						cfg->flying = TRUE;
						HANDLE thread = CreateThread(NULL, 0, flying, cfg, 0, NULL);
						break;
					}
					case (CMD_FLYING | CMD_ERROR):
					{
						//error saying to the control that the trip is starting
						sout("Error: '%s'\n", buffer.command.str);
						break;
					}
					case (CMD_FLYING | CMD_OK):
					{


						break;
					}
					case (CMD_FLYING | CMD_ERROR):
					{
						break;
					}
					default:
					{
						sout("Oops something went wrong!\n");
						break;
					}
				}
				//sout("Command: %d\nFrom: %u\nTo: %u\n\n", buffer.cmd_id, buffer.from_id, buffer.to_id);
			}
		} else {
			// if stop_event is triggered
			// if stop_airplane is triggered this is redundant
			cfg->die = TRUE;
			sout("System has been stopped!\n");
		}
	}

	return 0;
}

DWORD WINAPI send_heartbeat(void *param) {
	Config *cfg = (Config *) param;
	HANDLE handles[2];
	handles[0] = cfg->stop_event;
	handles[1] = cfg->stop_airplane;
	SharedBuffer buffer;
	buffer.from_id = cfg->airplane.pid;
	buffer.cmd_id = CMD_HEARTBEAT;
	while (!cfg->die && WaitForMultipleObjects(2, handles, FALSE, HEARTBEAT_TIMER) == WAIT_TIMEOUT) {
		send_command(cfg, &buffer);
	}

	return 0;
}

/*
* Wait for 3 handles
* stop_event = System has been shutdown
* stop_airplane = Airplane has been shutdown
* sem_itemA = There is a new item on the circular buffer
*
* Check if it's for this airplane
* if it is consume the item by incrementing the "out" index
* and releasing the sem_emptyA semaphore (meaning there is a new empty space)
* else release the sem_itemA semaphore (meaning the item has not been consumed)
*/
BOOL receive_command(Config *cfg, SharedBuffer *sb) {
	HANDLE handles[3];
	handles[0] = cfg->stop_event;
	handles[1] = cfg->stop_airplane;
	handles[2] = cfg->sem_itemA;
	DWORD res;
	do {
		if (!((res = WaitForMultipleObjects(3, handles, FALSE, INFINITE)) == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1))) {
			WaitForSingleObject(cfg->mtx_A, INFINITE);
			CopyMemory(sb, &(cfg->memory->bufferAirplane[cfg->memory->outA]), sizeof(SharedBuffer));
			if (sb->to_id == cfg->airplane.pid) {
				cfg->memory->outA = (cfg->memory->outA + 1) % MAX_SHARED_BUFFER;
			}
			ReleaseMutex(cfg->mtx_A);
			if (sb->to_id == cfg->airplane.pid) {
				ReleaseSemaphore(cfg->sem_emptyA, 1, NULL);
				return TRUE;
			} else {
				ReleaseSemaphore(cfg->sem_itemA, 1, NULL);
			}
		} else {
			return FALSE;
		}
	} while (TRUE);
}

BOOL send_command(Config *cfg, SharedBuffer *sb) {
	HANDLE handles[3];
	handles[0] = cfg->stop_event;
	handles[1] = cfg->stop_airplane;
	handles[2] = cfg->sem_emptyC;
	DWORD res;
	if (!((res = WaitForMultipleObjects(3, handles, FALSE, INFINITE)) == WAIT_OBJECT_0 || res == (WAIT_OBJECT_0 + 1))) {
		WaitForSingleObject(cfg->mtx_C, INFINITE);
		CopyMemory(&(cfg->memory->bufferControl[cfg->memory->inC]), sb, sizeof(SharedBuffer));
		cfg->memory->inC = (cfg->memory->inC + 1) % MAX_SHARED_BUFFER;
		ReleaseMutex(cfg->mtx_C);
		ReleaseSemaphore(cfg->sem_itemC, 1, NULL);
		return TRUE;
	}

	return FALSE;
}

DWORD WINAPI flying(void *param) {
	Config *cfg = (Config *) param;
	HANDLE handles[3];
	handles[0] = cfg->stop_event;
	handles[1] = cfg->stop_airplane;
	handles[2] = cfg->mtx_memory;
	SharedBuffer sb;
	sb.from_id = cfg->airplane.pid;
	sb.to_id = 0;
	int moved = 1;
	Point new_coord, old_coord, dest_coord;
	new_coord = (const Point){ 0 };
	do {
		old_coord = cfg->airplane.coordinates;
		dest_coord = cfg->airplane.airport_end.coordinates;
		for (int i = 0; i < cfg->airplane.velocity && moved == 1; i++) {
			moved = cfg->move(old_coord.x, old_coord.y,
				dest_coord.x, dest_coord.y,
				&new_coord.x, &new_coord.y);
		}
		if (moved == 0) {
			// Arrived
			if (WaitForMultipleObjects(3, handles, FALSE, INFINITE) == (WAIT_OBJECT_0 + 2)) {
				// Check previous coord if they can be erased
				if (cfg->memory->map[old_coord.x][old_coord.y] == cfg->airplane.pid) {
					cfg->memory->map[old_coord.x][old_coord.y] = 0;
				}
				ReleaseMutex(cfg->mtx_memory);
				cfg->airplane.coordinates = new_coord;
				// Send landed command
				sb.cmd_id = CMD_LANDED;
				sb.command.coordinates = cfg->airplane.coordinates;
				send_command(cfg, &sb);
				cfg->flying = FALSE;
			}
		} else if (moved == 1) {
			// Still flying
			if (WaitForMultipleObjects(3, handles, FALSE, INFINITE) == (WAIT_OBJECT_0 + 2)) {
				// Check next coords
				sb.cmd_id = CMD_FLYING;
				cfg->airplane.coordinates = new_coord;
				if (cfg->memory->map[new_coord.x][new_coord.y] > 0 && cfg->memory->map[new_coord.x][new_coord.y] <= cfg->memory->max_airport) {
					// Airport
				} else if (cfg->memory->map[new_coord.x][new_coord.y] == 0) {
					// Empty
					cfg->memory->map[new_coord.x][new_coord.y] = cfg->airplane.pid;
				} else {
					// Airplane
					sb.cmd_id = CMD_AVOID_COLLISION;
					cfg->airplane.coordinates = old_coord;
				}
				// Check old coords
				if (cfg->memory->map[old_coord.x][old_coord.y] == cfg->airplane.pid) {
					cfg->memory->map[old_coord.x][old_coord.y] = 0;
				}
				ReleaseMutex(cfg->mtx_memory);
				// Send flying command
				sb.command.coordinates = cfg->airplane.coordinates;
				send_command(cfg, &sb);
			}
		} else {
			// Error
			sout("An error occured while moving...\n");
		}
	} while (!cfg->die && cfg->flying && WaitForMultipleObjects(2, handles, FALSE, 1000) == WAIT_TIMEOUT);

	return 0;
}
