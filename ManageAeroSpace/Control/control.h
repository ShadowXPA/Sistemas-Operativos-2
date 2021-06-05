#ifndef CONTROL_H
#define CONTROL_H

#include "../Utils/utils.h"

#define AIRPORT_RADIUS 10

#define MTX_CTR _T("MTXControl")
#define MTX_AIRPORT _T("MTXAirport")
#define MTX_AIRPLANE _T("MTXAirplane")
#define MTX_PASSENGER _T("MTXPassenger")

#define REGISTRY_NAME _T("Software\\SO2\\TP")
#define REGISTRY_AIRPORT_KEY _T("MAX_AIRPORT")
#define REGISTRY_AIRPLANE_KEY _T("MAX_AIRPLANE")
#define REGISTRY_PASSENGER_KEY _T("MAX_PASSENGER")

#define MTX_C _T("ControlMutexC")
#define MTX_A _T("ControlMutexA")

typedef struct cfg {
	BOOL die;
	unsigned int max_airport;			// Maximum number of airports
	unsigned int max_airplane;			// Maximum number of airplanes
	unsigned int max_passenger;			// Maximum number of passengers
	SharedMemory *memory;				// MapViewOfFile
	//unsigned int next_airport_id;		// 1 <-> MAX_AIRPORT; ex: MAX_AIRPORT = 90
	//unsigned int next_airplane_id;		// MAX_AIRPORT + 1 <-> (MAX_AIRPORT + MAX_AIRPLANE); ex: MAX_AIRPLANE = 100
	//unsigned int next_passenger_id;		// (MAX_AIRPORT + MAX_AIRPLANE + 1) <-> (MAX_AIRPORT + MAX_AIRPLANE + MAX_PASSENGER); ex: MAX_PASSENGER = 100 * MAX_AIRPLANE
	//unsigned int map[MAX_MAP][MAX_MAP];	// IDs (0 = empty, 1 ~ 90 = airports, 91 ~ 190 = airplanes)
	Airport *airports;					// List of airports
	Airplane *airplanes;				// List of airplanes
	Passenger *passengers;				// List of passengers
	// Handles
	CRITICAL_SECTION cs_airport;		// Mutex for airport access
	CRITICAL_SECTION cs_airplane;		// Mutex for airplane access
	CRITICAL_SECTION cs_passenger;		// Mutex for passenger access
	HANDLE obj_map;
	HANDLE mtx_instance;				// Mutex used to verify various instances of the same program
	HANDLE mtx_memory;					// Mutex for shared memory access
	HANDLE stop_event;					// Kill event (Stops the whole system)
	/*
		// To send an item through circular a buffer use:
		// Handle the command
		// ...
		WaitForSingleObject(cfg->sem_emptyX, INFINITE);
		WaitForSingleObject(cfg->mtx_X, INFINITE);
		CopyMemory(&(cfg->memory->bufferX[cfg->memory->inX]), &command, sizeof(SharedBuffer));
		cfg->memory->inX = (cfg->memory->inX + 1) % MAX_SHARED_BUFFER;
		ReleaseMutex(cfg->mtx_X);
		ReleaseSemaphore(cfg->sem_itemX, 1, NULL);
		// ----------------------
		// To receive an item through circular buffer use:
		WaitForSingleObject(cfg->sem_itemX, INFINITE);
		WaitForSingleObject(cfg->mtx_X, INFINITE);
		CopyMemory(&command, &(cfg->memory->bufferX[cfg->memory->outX]), sizeof(SharedBuffer));
		cfg->memory->outX = (cfg->memory->outX + 1) % MAX_SHARED_BUFFER;
		ReleaseMutex(cfg->mtx_X);
		ReleaseSemaphore(cfg->sem_emptyX, 1, NULL);
		// Handle the command
		// ...
	*/
	HANDLE sem_emptyC;					// Semaphore for empty spots in BufferC
	HANDLE sem_itemC;					// Semaphore for items in BufferC
	HANDLE mtx_C;						// Mutex for BufferC
	HANDLE sem_emptyA;					// Semaphore for empty spots in BufferA
	HANDLE sem_itemA;					// Semaphore for items in BufferA
	HANDLE mtx_A;						// Mutex for BufferA
	// Pipe Handles
	OVERLAPPED overlapped;
	HANDLE ovr_event;
} Config;

BOOL init_config(Config *);
void end_config(Config *);
void init_control(Config *);

DWORD WINAPI read_command(void *);
DWORD WINAPI read_shared_memory(void *);
DWORD WINAPI read_named_pipes(void *);
DWORD WINAPI handle_heartbeat(void *);
DWORD WINAPI handle_single_passenger(void *);

typedef struct passengercfg {
	Config *cfg;
	Passenger *passenger;
} PassengerConfig;

void *get_by_id(Config *, unsigned int);
Airport *get_airport_by_id(Config *, unsigned int);
Airplane *get_airplane_by_id(Config *, unsigned int);
Airplane *get_airplane_by_pid(Config *, unsigned int);
Passenger *get_passenger_by_id(Config *, unsigned int);
Airport *get_available_airport(Config *);
Airplane *get_available_airplane(Config *);
Passenger *get_available_passenger(Config *);
Airport *get_airport_by_name(Config *, const TCHAR *);
Airport *get_airport_by_name_or_radius(Config *, const TCHAR *, const Point, const unsigned int radius);
Airplane *get_airplane_by_name(Config *, const TCHAR *);
Passenger *get_passenger_by_name(Config *, const TCHAR *);

BOOL add_airport(Config *, Airport *);
BOOL add_airplane(Config *, Airplane *);
BOOL add_passenger(Config *, Passenger *, Passenger *);
BOOL remove_airport(Config *, unsigned int);
BOOL remove_airplane(Config *, unsigned int);
BOOL _remove_airplane(Config *, Airplane *);
BOOL remove_passenger(Config *, unsigned int);

void print_airports(Config *);
void print_airplane(Config *);
void print_passenger(Config *);

BOOL receive_command_sharedmemory(Config *, SharedBuffer *);
BOOL send_command_sharedmemory(Config *, SharedBuffer *);
BOOL receive_message_namedpipe(PassengerConfig *, NamedPipeBuffer *);
BOOL send_message_namedpipe(PassengerConfig *, NamedPipeBuffer *);

#endif // !CONTROL_H
