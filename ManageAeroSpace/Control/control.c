#include "control.h"

int init_config(Config *cfg) {
	memset(cfg, 0, sizeof(Config));

	//cfg->mtx_instance = OpenMutex(MUTEX_ALL_ACCESS, TRUE, MTX_CTR);
	//if (cfg->mtx_instance == NULL) {
	//	cfg->mtx_instance = CreateMutex(NULL, TRUE, MTX_CTR);
	//} else {
	//	sout("Application already running\n");
	//	return FALSE;
	//}

	cfg->mtx_instance = CreateMutex(NULL, TRUE, MTX_CTR);
	DWORD already_exists = GetLastError();
	if (already_exists == ERROR_ALREADY_EXISTS) {
		sout("Application already running\n");
		return FALSE;
	}

	cfg->mtx_memory = CreateMutex(NULL, FALSE, MTX_MEMORY);
	if (cfg->mtx_memory == NULL)
		return FALSE;
	cfg->mtx_airport = CreateMutex(NULL, FALSE, MTX_AIRPORT);
	if (cfg->mtx_airport == NULL)
		return FALSE;
	cfg->mtx_airplane = CreateMutex(NULL, FALSE, MTX_AIRPLANE);
	if (cfg->mtx_airplane == NULL)
		return FALSE;
	cfg->mtx_passenger = CreateMutex(NULL, FALSE, MTX_PASSENGER);
	if (cfg->mtx_passenger == NULL)
		return FALSE;

	cfg->max_airport = MAX_AIRPORT;
	cfg->max_airplane = MAX_AIRPLANE;
	cfg->max_passenger = MAX_PASSENGER;
	// Get MAX_... from registry
	// If not set, set default values

	cfg->airports = calloc(cfg->max_airport, sizeof(Airport));
	cfg->airplanes = calloc(cfg->max_airplane, sizeof(Airplane));
	cfg->passengers = calloc(cfg->max_passenger, sizeof(Passenger));

	if (cfg->airports == NULL || cfg->airplanes == NULL || cfg->passengers == NULL) {
		return FALSE;
	}

	unsigned int index = 1;
	for (unsigned int i = 0; i < cfg->max_airport; i++) {
		cfg->airports[i].id = (index++);
	}
	for (unsigned int i = 0; i < cfg->max_airplane; i++) {
		cfg->airplanes[i].id = (index++);
	}
	for (unsigned int i = 0; i < cfg->max_passenger; i++) {
		cfg->passengers[i].id = (index++);
	}

	cfg->stop_event = CreateEvent(NULL, TRUE, FALSE, STOP_SYSTEM_EVENT);
	if (cfg->stop_event == NULL)
		return FALSE;

	return TRUE;
}

void end_config(Config *cfg) {
	free(cfg->airports);
	free(cfg->airplanes);
	free(cfg->passengers);
	UnmapViewOfFile(cfg->memory);
	CloseHandle(cfg->obj_map);
	ReleaseMutex(cfg->mtx_instance);
	CloseHandle(cfg->mtx_instance);
	CloseHandle(cfg->mtx_memory);
	CloseHandle(cfg->mtx_airport);
	CloseHandle(cfg->mtx_airplane);
	CloseHandle(cfg->mtx_passenger);
	CloseHandle(cfg->stop_event);
	memset(cfg, 0, sizeof(Config));
}

void init_control(Config *cfg) {
	// init window (Win32)

	// init shared memory
	cfg->obj_map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemory), FILE_MAPPING_NAME);
	if (cfg->obj_map == NULL)
		return;
	cfg->memory = MapViewOfFile(cfg->obj_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (cfg->memory == NULL)
		return;
	cfg->memory->accepting_planes = TRUE;
	// init named pipes
	// init threads:
	HANDLE thread[3];
	// read command
	thread[0] = CreateThread(NULL, 0, read_command, cfg, 0, NULL);
	if (thread[0] == NULL)
		return;
	// read shared memory
	thread[1] = CreateThread(NULL, 0, read_shared_memory, cfg, 0, NULL);
	if (thread[1] == NULL)
		return;
	// read named pipes
	thread[2] = CreateThread(NULL, 0, read_named_pipes, cfg, 0, NULL);
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

		if (icmp(buffer, "add") == 0) {
			// add airport
			sout("Input airport name:\n > ");
			sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);

			// check if maximum as been reached
			// check if already exists
		} else if (icmp(buffer, "toggle") == 0) {
			// toggle plane acceptance
			WaitForSingleObject(cfg->mtx_memory, INFINITE);
			cfg->memory->accepting_planes = !cfg->memory->accepting_planes;
			ReleaseMutex(cfg->mtx_memory);
			sout("%s", (cfg->memory->accepting_planes == TRUE ? _T("Accepting planes.\n") : _T("Not accepting planes.\n")));
		} else if (icmp(buffer, "list") == 0) {
			sout("What do you want to list:\n > ");
			sin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			if (icmp(buffer, "airport") == 0) {
				// list airports
				WaitForSingleObject(cfg->mtx_airport, INFINITE);
				print_airports(cfg);
				ReleaseMutex(cfg->mtx_airport);
			} else if (icmp(buffer, "airplane") == 0) {
				// list airplanes
				WaitForSingleObject(cfg->mtx_airplane, INFINITE);
				print_airplane(cfg);
				ReleaseMutex(cfg->mtx_airplane);
			} else if (icmp(buffer, "passenger") == 0) {
				// list passengers
				WaitForSingleObject(cfg->mtx_passenger, INFINITE);
				print_passenger(cfg);
				ReleaseMutex(cfg->mtx_passenger);
			} else if (icmp(buffer, "all") == 0) {
				WaitForSingleObject(cfg->mtx_airport, INFINITE);
				WaitForSingleObject(cfg->mtx_airplane, INFINITE);
				WaitForSingleObject(cfg->mtx_passenger, INFINITE);
				print_airports(cfg);
				print_airplane(cfg);
				print_passenger(cfg);
				ReleaseMutex(cfg->mtx_passenger);
				ReleaseMutex(cfg->mtx_airplane);
				ReleaseMutex(cfg->mtx_airport);
			}
		} else if (icmp(buffer, "help") == 0) {
			// show all commands
		} else {
			sout("Invalid command!\n");
		}
	} while (icmp(buffer, "exit") != 0);
	SetEvent(cfg->stop_event);
	return 0;
}

DWORD WINAPI read_shared_memory(void *param) {
	// Receive new airplane
	// Airplane is accepting passengers
	// Airplane is starting lift off
	// Airplane is moving (Coordinates)
	// Airplane is de-touring or waiting (Avoid collision)
	// Airplane has landed
	// Airplane has crashed or pilot retired
	// Airplane sends heartbeat
	// Send destination coordinates

	Config *cfg = (Config *) param;
	HANDLE handles[2];
	handles[0] = cfg->stop_event;
	WaitForSingleObject(handles[0], INFINITE);
	return 0;
}

DWORD WINAPI read_named_pipes(void *param) {
	Config *cfg = (Config *) param;
	WaitForSingleObject(cfg->stop_event, INFINITE);
	return 0;
}

void *get_by_id(Config *cfg, unsigned int id) {
	if (id < 1 || id >(cfg->max_airport + cfg->max_airplane + cfg->max_passenger))
		return NULL;

	if (id <= cfg->max_airport) {
		// Airport
		return &(cfg->airports[id - 1]);
	} else if (id <= (cfg->max_airport + cfg->max_airplane)) {
		// Airplane
		return &(cfg->airplanes[id - (cfg->max_airport + 1)]);
	} else {
		// Passenger
		return &(cfg->passengers[id - (cfg->max_airport + cfg->max_airplane + 1)]);
	}
}

Airport *get_airport_by_id(Config *cfg, unsigned int id) {
	if (id > cfg->max_airport)
		return NULL;

	return ((Airport *) get_by_id(cfg, id));
}

Airplane *get_airplane_by_id(Config *cfg, unsigned int id) {
	if (id < cfg->max_airport + 1 || id >(cfg->max_airport + cfg->max_airplane))
		return NULL;

	return ((Airplane *) get_by_id(cfg, id));
}

Passenger *get_passenger_by_id(Config *cfg, unsigned int id) {
	if (id < (cfg->max_airport + cfg->max_airplane + 1))
		return NULL;

	return ((Passenger *) get_by_id(cfg, id));
}

void print_airports(Config *cfg) {
	for (unsigned int i = 1; i <= cfg->max_airport; i++) {
		Airport *airport = get_airport_by_id(cfg, i);
		if (airport->active) {
			sout("Name: '%s' (%u)\nCoordinates: (%u, %u)\n\n", airport->name, airport->id, airport->coordinates.x, airport->coordinates.y);
		}
	}
}

void print_airplane(Config *cfg) {
	for (unsigned int i = (cfg->max_airport + 1); i <= (cfg->max_airport + cfg->max_airplane); i++) {
		Airplane *airplane = get_airplane_by_id(cfg, i);
		if (airplane->active) {
			Airport *departure = get_airport_by_id(cfg, airplane->airport_start);
			Airport *destination = get_airport_by_id(cfg, airplane->airport_end);
			sout("Name: '%s' (%u)\nVelocity: %d\nCapacity: %d\nMax. Capacity: %d\nCoordinates: (%u, %u)\nDeparture: '%s' (%u)\nDestination: '%s' (%u)\n\n",
				airplane->name, airplane->id, airplane->velocity, airplane->capacity, airplane->max_capacity, airplane->coordinates.x, airplane->coordinates.y,
				departure->name, departure->id, destination->name, destination->id);
		}
	}
}

void print_passenger(Config *cfg) {
	for (unsigned int i = (cfg->max_airport + cfg->max_airplane + 1); i <= (cfg->max_airport + cfg->max_airplane + cfg->max_passenger); i++) {
		Passenger *passenger = get_passenger_by_id(cfg, i);
		if (passenger->active) {
			Airport *destination = get_airport_by_id(cfg, passenger->airport_end);
			sout("Name: '%s' (%u)\nDestination: '%s' (%u)\n", passenger->name, passenger->id, destination->name, destination->id);
			if (passenger->airplane) {
				Airplane *airplane = get_airplane_by_id(cfg, passenger->airplane);
				sout("Flying on: '%s' (%u)\nCoordinates: (%u, %u)\n", airplane->name, airplane->id, airplane->coordinates.x, airplane->coordinates.y);
			} else {
				Airport *current_airport = get_airport_by_id(cfg, passenger->airport);
				sout("Waiting for airplane at: '%s' (%u)\nCoordinates: (%u, %u)\nWaiting time left: %d\n", current_airport->name, current_airport->id,
					current_airport->coordinates.x, current_airport->coordinates.y, passenger->wait_time);
			}
			sout("\n");
		}
	}
}

void clear_input_stream(const FILE *const p) {
	int ch;
	while ((ch = _gettc(p)) != '\n' && ch != EOF);
}
