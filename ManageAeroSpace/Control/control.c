#include "control.h"

BOOL init_config(Config *cfg) {
	memset(cfg, 0, sizeof(Config));

	//cfg->mtx_instance = OpenMutex(MUTEX_ALL_ACCESS, TRUE, MTX_CTR);
	//if (cfg->mtx_instance == NULL) {
	//	cfg->mtx_instance = CreateMutex(NULL, TRUE, MTX_CTR);
	//} else {
	//	cout("Application already running\n");
	//	return FALSE;
	//}

	cfg->mtx_instance = CreateMutex(NULL, TRUE, MTX_CTR);
	DWORD already_exists = GetLastError();
	if (already_exists == ERROR_ALREADY_EXISTS) {
		cout("Application already running\n");
		return FALSE;
	}

	cfg->mtx_memory = CreateMutex(NULL, FALSE, MTX_MEMORY);
	if (cfg->mtx_memory == NULL)
		return FALSE;

	InitializeCriticalSection(&cfg->cs_airport);
	InitializeCriticalSection(&cfg->cs_airplane);
	InitializeCriticalSection(&cfg->cs_passenger);

	cfg->max_airport = MAX_AIRPORT;
	cfg->max_airplane = MAX_AIRPLANE;
	cfg->max_passenger = MAX_PASSENGER;
	// Get MAX_... from registry
	// If not set, set default values
	HKEY key;
	DWORD result;
	LSTATUS res = RegCreateKeyEx(HKEY_CURRENT_USER, REGISTRY_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &result);
	if (res != ERROR_SUCCESS)
		return FALSE;
	if (result == REG_CREATED_NEW_KEY) {
		// Created registry key
		// Add default values
		res = RegSetValueEx(key, REGISTRY_AIRPORT_KEY, 0, REG_DWORD, (const BYTE *) &cfg->max_airport, sizeof(DWORD));
		if (res != ERROR_SUCCESS) {
			return FALSE;
		}
		res = RegSetValueEx(key, REGISTRY_AIRPLANE_KEY, 0, REG_DWORD, (const BYTE *) &cfg->max_airplane, sizeof(DWORD));
		if (res != ERROR_SUCCESS) {
			return FALSE;
		}
		res = RegSetValueEx(key, REGISTRY_PASSENGER_KEY, 0, REG_DWORD, (const BYTE *) &cfg->max_passenger, sizeof(DWORD));
		if (res != ERROR_SUCCESS) {
			return FALSE;
		}
	} else {
		// Opened registry key
		// Get default values
		DWORD max;
		DWORD size = sizeof(DWORD);
		DWORD type = REG_DWORD;
		res = RegQueryValueEx(key, REGISTRY_AIRPORT_KEY, 0, &type, (LPBYTE) &max, &size);
		if (res == ERROR_SUCCESS) {
			cfg->max_airport = max;
		}
		res = RegQueryValueEx(key, REGISTRY_AIRPLANE_KEY, 0, &type, (LPBYTE) &max, &size);
		if (res == ERROR_SUCCESS) {
			cfg->max_airplane = max;
		}
		res = RegQueryValueEx(key, REGISTRY_PASSENGER_KEY, 0, &type, (LPBYTE) &max, &size);
		if (res == ERROR_SUCCESS) {
			cfg->max_passenger = max;
		}
	}

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

	cfg->sem_emptyC = CreateSemaphore(NULL, MAX_SHARED_BUFFER, MAX_SHARED_BUFFER, SEM_EMPTY_C);
	cfg->sem_emptyA = CreateSemaphore(NULL, MAX_SHARED_BUFFER, MAX_SHARED_BUFFER, SEM_EMPTY_A);
	cfg->sem_itemC = CreateSemaphore(NULL, 0, MAX_SHARED_BUFFER, SEM_ITEM_C);
	cfg->sem_itemA = CreateSemaphore(NULL, 0, MAX_SHARED_BUFFER, SEM_ITEM_A);
	if (cfg->sem_emptyC == NULL || cfg->sem_emptyA == NULL || cfg->sem_itemC == NULL || cfg->sem_itemA == NULL)
		return FALSE;

	cfg->mtx_C = CreateMutex(NULL, FALSE, MTX_C);
	cfg->mtx_A = CreateMutex(NULL, FALSE, MTX_A);
	if (cfg->mtx_C == NULL || cfg->mtx_A == NULL)
		return FALSE;

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
	DeleteCriticalSection(&cfg->cs_airport);
	DeleteCriticalSection(&cfg->cs_airport);
	DeleteCriticalSection(&cfg->cs_airport);
	CloseHandle(cfg->sem_emptyC);
	CloseHandle(cfg->sem_emptyA);
	CloseHandle(cfg->sem_itemC);
	CloseHandle(cfg->sem_itemA);
	CloseHandle(cfg->mtx_C);
	CloseHandle(cfg->mtx_A);
	CloseHandle(cfg->stop_event);
	memset(cfg, 0, sizeof(Config));
}

void init_control(Config *cfg) {
	// init window (Win32)

	// init shared memory
	cfg->obj_map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemory), FILE_MAPPING_NAME);
	if (cfg->obj_map == NULL)
		return;
	cfg->memory = (SharedMemory *) MapViewOfFile(cfg->obj_map, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (cfg->memory == NULL)
		return;
	cfg->memory->accepting_planes = TRUE;
	cfg->memory->max_airport = cfg->max_airport;
	// init named pipes
	// init threads:
	HANDLE thread[4];
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
	// handle heartbeat from airplanes
	thread[3] = CreateThread(NULL, 0, handle_heartbeat, cfg, 0, NULL);
	if (thread[3] == NULL)
		return;

	WaitForMultipleObjects(4, thread, TRUE, INFINITE);

	CloseHandle(thread[0]);
	CloseHandle(thread[1]);
	CloseHandle(thread[2]);
	CloseHandle(thread[3]);
}

DWORD WINAPI read_command(void *param) {
	Config *cfg = (Config *) param;
	TCHAR buffer[MAX_NAME] = { 0 };
	do {
		cout("Input command:\n > ");
		cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);

		if (icmp(buffer, "add") == 0) {
			// add airport
			Airport airport;
			cout("Input airport name:\n > ");
			cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			_cpy(airport.name, buffer, MAX_NAME);
			cout("Input x coordinate:\n > ");
			cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			airport.coordinates.x = (_tstoi(buffer) - 1);
			cout("Input y coordinate:\n > ");
			cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			airport.coordinates.y = (_tstoi(buffer) - 1);

			EnterCriticalSection(&cfg->cs_airport);
			if (add_airport(cfg, &airport)) {
				cout("Airport added!\n");
			} else {
				cout("Airport not added!\n");
			}
			LeaveCriticalSection(&cfg->cs_airport);
		} else if (icmp(buffer, "remove") == 0) {
			cout("Input airport ID:\n > ");
			cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);

			EnterCriticalSection(&cfg->cs_airport);
			if (remove_airport(cfg, _tstoi(buffer))) {
				cout("Airport removed!\n");
			} else {
				cout("Airport not removed!\n");
			}
			LeaveCriticalSection(&cfg->cs_airport);
		} else if (icmp(buffer, "toggle") == 0) {
			// toggle plane acceptance
			WaitForSingleObject(cfg->mtx_memory, INFINITE);
			cfg->memory->accepting_planes = !cfg->memory->accepting_planes;
			ReleaseMutex(cfg->mtx_memory);
			cout("%s", (cfg->memory->accepting_planes ? _T("Accepting airplanes.\n") : _T("Not accepting airplanes.\n")));
		} else if (icmp(buffer, "list") == 0) {
			cout("What do you want to list:\n > ");
			cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			cout("\n");
			if (icmp(buffer, "airport") == 0) {
				// list airports
				EnterCriticalSection(&cfg->cs_airport);
				print_airports(cfg);
				LeaveCriticalSection(&cfg->cs_airport);
			} else if (icmp(buffer, "airplane") == 0) {
				// list airplanes
				EnterCriticalSection(&cfg->cs_airplane);
				print_airplane(cfg);
				LeaveCriticalSection(&cfg->cs_airplane);
			} else if (icmp(buffer, "passenger") == 0) {
				// list passengers
				EnterCriticalSection(&cfg->cs_passenger);
				print_passenger(cfg);
				LeaveCriticalSection(&cfg->cs_passenger);
			} else if (icmp(buffer, "all") == 0) {
				EnterCriticalSection(&cfg->cs_airport);
				EnterCriticalSection(&cfg->cs_airplane);
				EnterCriticalSection(&cfg->cs_passenger);
				print_airports(cfg);
				print_airplane(cfg);
				print_passenger(cfg);
				LeaveCriticalSection(&cfg->cs_passenger);
				LeaveCriticalSection(&cfg->cs_airplane);
				LeaveCriticalSection(&cfg->cs_airport);
			}
		} else if (icmp(buffer, "kick") == 0) {
			cout("Input airplane ID:\n > ");
			cin(DEFAULT_CIN_BUFFER, buffer, MAX_NAME);
			int id = _tstoi(buffer);

			EnterCriticalSection(&cfg->cs_airplane);
			Airplane *airplane = get_airplane_by_id(cfg, id);
			BOOL removed = FALSE;
			int pid = 0;
			if (airplane != NULL && airplane->active) {
				airplane->alive = 0;
				pid = airplane->pid;
				removed = _remove_airplane(cfg, airplane);
			} else {
				cout("Airplane does not exist!\n");
			}
			LeaveCriticalSection(&cfg->cs_airplane);
			if (removed) {
				SharedBuffer sb;
				sb.cmd_id = CMD_SHUTDOWN;
				sb.to_id = pid;
				sb.from_id = 0;
				send_command(cfg, &sb);
				cout("Airplane (ID: %u, PID: %u) removed!\n", id, pid);
			} else {
				cout("Airplane (ID: %u) not removed!\n", id);
			}
		} else if (icmp(buffer, "help") == 0) {
			// show all commands
			cout("help   -> Shows this\n");
			cout("add    -> Adds a new airport\n");
			cout("remove -> Removes an airport\n");
			cout("toggle -> Toggles between accepting airplanes or not\n");
			cout("list   -> Prints a list of airports, airplanes, passengers or all\n");
			cout("cfg    -> View config\n");
			cout("kick   -> Kicks an airplane\n");
			cout("exit   -> Stops the whole system\n");
		} else if (icmp(buffer, "exit") == 0) {
			cout("Stopping system...\n");
			cfg->die = TRUE;
		} else if (icmp(buffer, "cfg") == 0) {
			cout("Max. Airport: %u\nMax. Airplane: %u\nMax. Passenger: %u\n", cfg->max_airport, cfg->max_airplane, cfg->max_passenger);
			cout("Memory: %p\n", cfg->memory);
			cout("MTXMemory: %p\n", &cfg->mtx_memory);
		} else {
			cout("Invalid command!\n");
		}
	} while (!cfg->die);
	SetEvent(cfg->stop_event);
	return 0;
}

DWORD WINAPI read_shared_memory(void *param) {
	Config *cfg = (Config *) param;
	SharedBuffer buffer;
	while (!cfg->die) {
		// Receive new airplane										// DONE
		// Airplane is accepting passengers							// TODO
		// Airplane is starting lift off							// DONE
		// Airplane is moving (Coordinates)							// DONE
		// Airplane is de-touring or waiting (Avoid collision)		// DONE
		// Airplane has landed										// DONE
		// Airplane has crashed or pilot retired					// DONE
		// Airplane sends heartbeat									// DONE
		// Send destination coordinates								// DONE
		if (receive_command(cfg, &buffer)) {
			switch (buffer.cmd_id) {
				case CMD_HELLO:
				{
					EnterCriticalSection(&cfg->cs_airport);
					EnterCriticalSection(&cfg->cs_airplane);
					if (add_airplane(cfg, &buffer.command.airplane)) {
						buffer.cmd_id |= CMD_OK;
						cout("A new airplane has been registered!\n");
						Airplane ap = buffer.command.airplane;
						cout("Airplane: '%s' (ID: %u, PID: %u)\n", ap.name, ap.id, ap.pid);
						cout("Located in: '%s' (x: %u, y: %u)\n", ap.airport_start.name, ap.airport_start.coordinates.x, ap.airport_start.coordinates.y);
					} else {
						buffer.cmd_id |= CMD_ERROR;
						cpy(buffer.command.str, "Could not add airplane!", MAX_NAME);
					}
					buffer.to_id = buffer.from_id;
					buffer.from_id = 0;
					send_command(cfg, &buffer);
					LeaveCriticalSection(&cfg->cs_airplane);
					LeaveCriticalSection(&cfg->cs_airport);
					break;
				}
				case CMD_SEND_DESTINATION:
				{
					EnterCriticalSection(&cfg->cs_airport);
					EnterCriticalSection(&cfg->cs_airplane);
					Airport *airport = get_airport_by_name(cfg, buffer.command.airport.name);
					if (airport != NULL && airport->active) {
						Airplane *airplane = get_airplane_by_pid(cfg, buffer.from_id);
						if (airplane != NULL && airplane->active) {
							if (_icmp(airplane->airport_start.name, buffer.command.airport.name) == 0) {
								buffer.cmd_id |= CMD_ERROR;
								cpy(buffer.command.str, "Can not add departure as destination!", MAX_NAME);
							} else {
								// set airport to airplane
								buffer.cmd_id |= CMD_OK;
								airplane->airport_end = *airport;
								buffer.command.airport = *airport;
							}
						} else {
							buffer.cmd_id |= CMD_ERROR;
							cpy(buffer.command.str, "Airplane does not exist!", MAX_NAME);
						}
					} else {
						buffer.cmd_id |= CMD_ERROR;
						cpy(buffer.command.str, "Airport does not exist!", MAX_NAME);
					}
					buffer.to_id = buffer.from_id;
					buffer.from_id = 0;
					send_command(cfg, &buffer);
					LeaveCriticalSection(&cfg->cs_airplane);
					LeaveCriticalSection(&cfg->cs_airport);
					break;
				}
				case CMD_LIFT_OFF:
				{
					EnterCriticalSection(&cfg->cs_airplane);
					Airplane *airplane = get_airplane_by_pid(cfg, buffer.from_id);
					if (airplane != NULL && airplane->active) {
						buffer.cmd_id |= CMD_OK;
						cout("Airplane '%s' is taking off.\nStarting at '%s' at (x: %u, y: %u)\nGoing to '%s' at (x: %u, y: %u)\n",
							airplane->name, airplane->airport_start.name,
							airplane->airport_start.coordinates.x, airplane->airport_start.coordinates.y,
							airplane->airport_end.name,
							airplane->airport_end.coordinates.x, airplane->airport_end.coordinates.y);
					} else {
						buffer.cmd_id |= CMD_ERROR;
						cpy(buffer.command.str, "Airplane does not exist!", MAX_NAME);
					}
					buffer.to_id = buffer.from_id;
					buffer.from_id = 0;
					send_command(cfg, &buffer);
					LeaveCriticalSection(&cfg->cs_airplane);
					break;
				}
				case CMD_FLYING:
				{
					// Airplane is still flying
					EnterCriticalSection(&cfg->cs_airplane);
					Airplane *airplane = get_airplane_by_id(cfg, buffer.command.airplane.id);
					if (airplane != NULL && airplane->active) {
						airplane->coordinates = buffer.command.airplane.coordinates;
						cout("Airplane '%s' (ID: %u, PID: %u) has moved to (x: %u, y: %u)\n",
							airplane->name, airplane->id, airplane->pid, airplane->coordinates.x, airplane->coordinates.y);
					}
					LeaveCriticalSection(&cfg->cs_airplane);
					break;
				}
				case CMD_AVOID_COLLISION:
				{
					// Airplane is avoiding a collision
					EnterCriticalSection(&cfg->cs_airplane);
					Airplane *airplane = get_airplane_by_id(cfg, buffer.command.airplane.id);
					if (airplane != NULL && airplane->active) {
						airplane->coordinates = buffer.command.airplane.coordinates;
						cout("Airplane '%s' (ID: %u, PID: %u) avoided a collision by waiting at (x: %u, y: %u)\n",
							airplane->name, airplane->id, airplane->pid, airplane->coordinates.x, airplane->coordinates.y);
					}
					LeaveCriticalSection(&cfg->cs_airplane);
					break;
				}
				case CMD_LANDED:
				{
					// Airplane has landed
					EnterCriticalSection(&cfg->cs_airplane);
					Airplane *airplane = get_airplane_by_id(cfg, buffer.command.airplane.id);
					if (airplane != NULL && airplane->active) {
						airplane->coordinates = buffer.command.airplane.coordinates;
						cout("Airplane '%s' (ID: %u, PID: %u) has arrived at its destination at (x: %u, y: %u)\n",
							airplane->name, airplane->id, airplane->pid, airplane->coordinates.x, airplane->coordinates.y);
						airplane->airport_start = airplane->airport_end;
						airplane->airport_end = (const Airport){ 0 };
						buffer.cmd_id |= CMD_OK;
						buffer.to_id = buffer.from_id;
						buffer.from_id = 0;
						buffer.command.airplane = *airplane;
						send_command(cfg, &buffer);
					}
					LeaveCriticalSection(&cfg->cs_airplane);
					break;
				}
				case CMD_CRASHED_RETIRED:
				{
					// Airplane crashed (1) or retired (0)
					EnterCriticalSection(&cfg->cs_airplane);
					Airplane *airplane = get_airplane_by_pid(cfg, buffer.from_id);
					if (airplane != NULL) {
						if (buffer.command.number) {
							// Airplane was flying therefore it crashed
							cout("Airplane '%s' (ID: %u, PID: %u) has crashed on its way to '%s' (x: %u, y: %u) at position (x: %u, y: %u)!\n",
								airplane->name, airplane->id, airplane->pid, airplane->airport_end.name, airplane->airport_end.coordinates.x,
								airplane->airport_end.coordinates.y, airplane->coordinates.x, airplane->coordinates.y);
							// TODO Send crash message to passengers on the airplane
						} else {
							// Airplane was stationed at an airport therefore the pilot retired
							cout("Pilot from airplane '%s' (ID: %u, PID: %u) has retired.\n", airplane->name, airplane->id, airplane->pid);
						}
						airplane->alive = 0;
						_remove_airplane(cfg, airplane);
					}
					LeaveCriticalSection(&cfg->cs_airplane);
					break;
				}
				case CMD_BOARD:
				{
					// TODO
					break;
				}
				case CMD_HEARTBEAT:
				{
					EnterCriticalSection(&cfg->cs_airplane);
					Airplane *airplane = get_airplane_by_pid(cfg, buffer.from_id);
					if (airplane != NULL && airplane->active)
						airplane->alive = 1;
					LeaveCriticalSection(&cfg->cs_airplane);
					break;
				}
				default:
					cout("[SharedMemory] Invalid Command!\n");
					break;
			}
		}
	}
	return 0;
}

BOOL receive_command(Config *cfg, SharedBuffer *sb) {
	HANDLE handles[2];
	handles[0] = cfg->stop_event;
	handles[1] = cfg->sem_itemC;
	if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) != WAIT_OBJECT_0) {
		WaitForSingleObject(cfg->mtx_C, INFINITE);
		CopyMemory(sb, &(cfg->memory->bufferControl[cfg->memory->outC]), sizeof(SharedBuffer));
		cfg->memory->outC = (cfg->memory->outC + 1) % MAX_SHARED_BUFFER;
		ReleaseMutex(cfg->mtx_C);
		ReleaseSemaphore(cfg->sem_emptyC, 1, NULL);
		return TRUE;
	}

	return FALSE;
}

BOOL send_command(Config *cfg, SharedBuffer *sb) {
	HANDLE handles[2];
	handles[0] = cfg->stop_event;
	handles[1] = cfg->sem_emptyA;
	if (WaitForMultipleObjects(2, handles, FALSE, INFINITE) != WAIT_OBJECT_0) {
		WaitForSingleObject(cfg->mtx_A, INFINITE);
		CopyMemory(&(cfg->memory->bufferAirplane[cfg->memory->inA]), sb, sizeof(SharedBuffer));
		cfg->memory->inA = (cfg->memory->inA + 1) % MAX_SHARED_BUFFER;
		ReleaseMutex(cfg->mtx_A);
		ReleaseSemaphore(cfg->sem_itemA, 1, NULL);
		return TRUE;
	}

	return FALSE;
}

DWORD WINAPI read_named_pipes(void *param) {
	Config *cfg = (Config *) param;
	WaitForSingleObject(cfg->stop_event, INFINITE);
	return 0;
}

DWORD WINAPI handle_heartbeat(void *param) {
	Config *cfg = (Config *) param;
	do {
		EnterCriticalSection(&cfg->cs_airplane);
		for (unsigned int i = cfg->max_airport + 1; i < (cfg->max_airport + cfg->max_airplane); i++) {
			Airplane *airplane = get_airplane_by_id(cfg, i);
			if (airplane != NULL && airplane->active) {
				airplane->alive = 0;
			}
		}
		LeaveCriticalSection(&cfg->cs_airplane);
		if (WaitForSingleObject(cfg->stop_event, HEARTBEAT_TIMER) == WAIT_TIMEOUT) {
			EnterCriticalSection(&cfg->cs_airplane);
			for (unsigned int i = cfg->max_airport + 1; i < (cfg->max_airport + cfg->max_airplane); i++) {
				if (_remove_airplane(cfg, get_airplane_by_id(cfg, i))) {
					cout("[Heartbeat] Airplane (ID: %u) has been removed!\n", i);
				}
			}
			LeaveCriticalSection(&cfg->cs_airplane);
		}
	} while (!cfg->die);
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

Airplane *get_airplane_by_pid(Config *cfg, unsigned int pid) {
	for (unsigned int i = (cfg->max_airport + 1); i <= (cfg->max_airport + cfg->max_airplane); i++) {
		Airplane *airplane = get_airplane_by_id(cfg, i);
		if (airplane != NULL && airplane->pid == pid) {
			return airplane;
		}
	}

	return NULL;
}

Passenger *get_passenger_by_id(Config *cfg, unsigned int id) {
	if (id < (cfg->max_airport + cfg->max_airplane + 1))
		return NULL;

	return ((Passenger *) get_by_id(cfg, id));
}

Airport *get_available_airport(Config *cfg) {
	for (unsigned int i = 1; i <= cfg->max_airport; i++) {
		Airport *airport = get_airport_by_id(cfg, i);
		if (airport != NULL && !airport->active) {
			return airport;
		}
	}
	return NULL;
}

Airplane *get_available_airplane(Config *cfg) {
	for (unsigned int i = (cfg->max_airport + 1); i <= (cfg->max_airport + cfg->max_airplane); i++) {
		Airplane *airplane = get_airplane_by_id(cfg, i);
		if (airplane != NULL && !airplane->active) {
			return airplane;
		}
	}

	return NULL;
}

Passenger *get_available_passenger(Config *cfg) {
	for (unsigned int i = (cfg->max_airport + cfg->max_airplane + 1); i <= (cfg->max_airport + cfg->max_airplane + cfg->max_passenger); i++) {
		Passenger *passenger = get_passenger_by_id(cfg, i);
		if (passenger != NULL && !passenger->active) {
			return passenger;
		}
	}

	return NULL;
}

Airport *get_airport_by_name(Config *cfg, const TCHAR *name) {
	for (unsigned int i = 1; i <= cfg->max_airport; i++) {
		Airport *airport = get_airport_by_id(cfg, i);
		if (airport != NULL && airport->active && _icmp(airport->name, name) == 0) {
			return airport;
		}
	}

	return NULL;
}

Airport *get_airport_by_name_or_radius(Config *cfg, const TCHAR *name, const Point coord, const unsigned int radius) {
	for (unsigned int i = 1; i <= cfg->max_airport; i++) {
		Airport *airport = get_airport_by_id(cfg, i);
		Point p;
		p.x = (unsigned int) abs(airport->coordinates.x - coord.x);
		p.y = (unsigned int) abs(airport->coordinates.y - coord.y);
		if (airport != NULL && airport->active && (_icmp(airport->name, name) == 0 ||
			((p.x <= radius) && (p.y <= radius)))) {
			return airport;
		}
	}

	return NULL;
}

Airplane *get_airplane_by_name(Config *cfg, const TCHAR *name) {
	for (unsigned int i = (cfg->max_airport + 1); i <= (cfg->max_airport + cfg->max_airplane); i++) {
		Airplane *airplane = get_airplane_by_id(cfg, i);
		if (airplane != NULL && airplane->active && _icmp(airplane->name, name) == 0) {
			return airplane;
		}
	}

	return NULL;
}

Passenger *get_passenger_by_name(Config *cfg, const TCHAR *name) {
	for (unsigned int i = (cfg->max_airport + cfg->max_airplane + 1); i <= (cfg->max_airport + cfg->max_airplane + cfg->max_passenger); i++) {
		Passenger *passenger = get_passenger_by_id(cfg, i);
		if (passenger != NULL && passenger->active && _icmp(passenger->name, name) == 0) {
			return passenger;
		}
	}

	return NULL;
}

BOOL add_airport(Config *cfg, Airport *airport) {
	Airport *tmp = get_available_airport(cfg);
	// check if maximum has been reached
	if (tmp == NULL)
		return FALSE;
	// check if coordinates are valid
	if (airport->coordinates.x >= MAX_MAP || airport->coordinates.y >= MAX_MAP)
		return FALSE;
	// check if name already exists or if any airport is too close to the airport
	if (get_airport_by_name_or_radius(cfg, airport->name, airport->coordinates, AIRPORT_RADIUS) != NULL)
		return FALSE;

	// Check map
	WaitForSingleObject(cfg->mtx_memory, INFINITE);
	unsigned int map_id = cfg->memory->map[airport->coordinates.x][airport->coordinates.y];
	if (map_id != 0) {
		ReleaseMutex(cfg->mtx_memory);
		return FALSE;
	}
	cfg->memory->map[airport->coordinates.x][airport->coordinates.y] = tmp->id;
	ReleaseMutex(cfg->mtx_memory);

	tmp->active = 1;
	tmp->coordinates = airport->coordinates;
	_cpy(tmp->name, airport->name, MAX_NAME);

	*airport = *tmp;

	return TRUE;
}

BOOL add_airplane(Config *cfg, Airplane *airplane) {
	Airplane *tmp = get_available_airplane(cfg);
	// check if maximum has been reached
	if (tmp == NULL)
		return FALSE;
	// check if name already exists
	if (_tcsnlen(airplane->name, MAX_NAME) == 0 || get_airplane_by_name(cfg, airplane->name) != NULL)
		return FALSE;

	Airport *airport = get_airport_by_id(cfg, airplane->airport_start.id);
	// index out of bounds?
	if (airport == NULL)
		return FALSE;
	// airport exists?
	if (!airport->active)
		return FALSE;

	tmp->active = 1;
	tmp->alive = 1;
	tmp->pid = airplane->pid;
	_cpy(tmp->name, airplane->name, MAX_NAME);
	tmp->max_capacity = airplane->max_capacity;
	tmp->capacity = 0;
	tmp->velocity = airplane->velocity;
	tmp->coordinates = airport->coordinates;
	tmp->airport_start = *airport;
	tmp->airport_end = (const Airport){ 0 };

	*airplane = *tmp;

	return TRUE;
}

BOOL add_passenger(Config *cfg, Passenger *passenger) {
	Passenger *tmp = get_available_passenger(cfg);
	// check if maximum has been reached
	if (tmp == NULL)
		return FALSE;
	// check if name already exists
	if (get_passenger_by_name(cfg, passenger->name) != NULL)
		return FALSE;

	*passenger = *tmp;

	return TRUE;
}

BOOL remove_airport(Config *cfg, unsigned int id) {
	Airport *airport = get_airport_by_id(cfg, id);
	if (airport != NULL && airport->active) {
		WaitForSingleObject(cfg->mtx_memory, INFINITE);
		cfg->memory->map[airport->coordinates.x][airport->coordinates.y] = 0;
		ReleaseMutex(cfg->mtx_memory);
		memset(airport, 0, sizeof(Airport));
		airport->id = id;
		return TRUE;
	}

	return FALSE;
}

BOOL remove_airplane(Config *cfg, unsigned int pid) {
	return _remove_airplane(cfg, get_airplane_by_pid(cfg, pid));

	//Airplane *airplane = get_airplane_by_pid(cfg, pid);
	//if (airplane != NULL && airplane->active) {
	//	WaitForSingleObject(cfg->mtx_memory, INFINITE);
	//	if (cfg->memory->map[airplane->coordinates.x][airplane->coordinates.y] == airplane->pid) {
	//		cfg->memory->map[airplane->coordinates.x][airplane->coordinates.y] = 0;
	//	}
	//	ReleaseMutex(cfg->mtx_memory);
	//	unsigned int tmp_id = airplane->id;
	//	memset(airplane, 0, sizeof(Airplane));
	//	airplane->id = tmp_id;
	//	return TRUE;
	//}

	//return FALSE;
}

BOOL _remove_airplane(Config *cfg, Airplane *airplane) {
	if (airplane != NULL && (airplane->active && !airplane->alive)) {
		WaitForSingleObject(cfg->mtx_memory, INFINITE);
		if (cfg->memory->map[airplane->coordinates.x][airplane->coordinates.y] == airplane->pid) {
			cfg->memory->map[airplane->coordinates.x][airplane->coordinates.y] = 0;
		}
		ReleaseMutex(cfg->mtx_memory);
		unsigned int tmp_id = airplane->id;
		memset(airplane, 0, sizeof(Airplane));
		airplane->id = tmp_id;
		return TRUE;
	}

	return FALSE;
}

BOOL remove_passenger(Config *cfg, unsigned int id) {
	return FALSE;
}

void print_airports(Config *cfg) {
	for (unsigned int i = 1; i <= cfg->max_airport; i++) {
		Airport *airport = get_airport_by_id(cfg, i);
		if (airport != NULL && airport->active) {
			cout("Name: '%s' (ID: %u)\nCoordinates: (x: %u, y: %u)\n\n", airport->name, airport->id, airport->coordinates.x, airport->coordinates.y);
		}
	}
}

void print_airplane(Config *cfg) {
	for (unsigned int i = (cfg->max_airport + 1); i <= (cfg->max_airport + cfg->max_airplane); i++) {
		Airplane *airplane = get_airplane_by_id(cfg, i);
		if (airplane != NULL && airplane->active) {
			Airport departure = airplane->airport_start;
			Airport destination = airplane->airport_end;
			cout("Name: '%s' (ID: %u, PID: %u)\nVelocity: %d\nCapacity: %d\nMax. Capacity: %d\nCoordinates: (x: %u, y: %u)\nDeparture: '%s' (ID: %u)\nDestination: '%s' (ID: %u)\n\n",
				airplane->name, airplane->id, airplane->pid, airplane->velocity, airplane->capacity, airplane->max_capacity, airplane->coordinates.x, airplane->coordinates.y,
				departure.name, departure.id, destination.name, destination.id);
		}
	}
}

void print_passenger(Config *cfg) {
	for (unsigned int i = (cfg->max_airport + cfg->max_airplane + 1); i <= (cfg->max_airport + cfg->max_airplane + cfg->max_passenger); i++) {
		Passenger *passenger = get_passenger_by_id(cfg, i);
		if (passenger != NULL && passenger->active) {
			Airport destination = passenger->airport_end;
			cout("Name: '%s' (ID: %u)\nDestination: '%s' (ID: %u)\n", passenger->name, passenger->id, destination.name, destination.id);
			if (passenger->airplane.pid) {
				Airplane airplane = passenger->airplane;
				cout("Flying on: '%s' (ID: %u, PID: %u)\nCoordinates: (x: %u, y: %u)\n", airplane.name, airplane.id, airplane.pid, airplane.coordinates.x, airplane.coordinates.y);
			} else {
				Airport current_airport = passenger->airport;
				cout("Waiting for airplane at: '%s' (ID: %u)\nCoordinates: (x: %u, y: %u)\nWaiting time left: %d\n", current_airport.name, current_airport.id,
					current_airport.coordinates.x, current_airport.coordinates.y, passenger->wait_time);
			}
			cout("\n");
		}
	}
}
