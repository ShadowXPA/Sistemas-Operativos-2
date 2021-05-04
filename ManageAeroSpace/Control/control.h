#ifndef CONTROL_H
#define CONTROL_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>
#include "../Utils/utils.h"

//void clear_input_stream(const FILE *const p);
//
//#define _sout(p, x, ...) _ftprintf_s(p, _T(x), __VA_ARGS__)
//#define sout(x, ...) _sout(stdout, x, __VA_ARGS__)
//#define _sin(p, x, ...) {\
//							_ftscanf_s(p, _T(x), __VA_ARGS__);\
//							clear_input_stream(p);\
//						}
//#define sin(x, ...) _sin(stdin, x, __VA_ARGS__)
//#define cmp(str1, str2) _tcscmp(str1, _T(str2))
//#define icmp(str1, str2) _tcsicmp(str1, _T(str2))
//#define contains(str, search) _tcsstr(str, _T(search))

//#define MAX_MAP 1000
//#define MAX_NAME 50
//#define MAX_SHARED_BUFFER 20
//
//#define MAX_AIRPORT 90
//#define MAX_AIRPLANE 100
//#define MAX_PASSENGER (100 * MAX_AIRPLANE)
//
//#define DEFAULT_CIN_BUFFER "%49[^\n]"
//
#define MTX_CTR _T("MTXControl")
//#define MTX_MEMORY _T("MTXSharedMemory")
#define MTX_AIRPORT _T("MTXAirport")
#define MTX_AIRPLANE _T("MTXAirplane")
#define MTX_PASSENGER _T("MTXPassenger")
//#define FILE_MAPPING_NAME _T("ControlAviao")
//#define STOP_SYSTEM_EVENT _T("StopEvent")

//typedef struct point {
//	unsigned int x, y;
//} Point;
//
//typedef struct airport {
//	unsigned int id;					// 1 ~ 90
//	unsigned int active : 1;
//	TCHAR name[MAX_NAME];
//	Point coordinates;					// 0 ~ 1000
//} Airport;
//
//typedef struct airplane {
//	unsigned int id;					// 91 ~ 190, MAYBE ADD PID?
//	unsigned int active : 1;
//	TCHAR name[MAX_NAME];
//	int velocity;
//	int capacity;						// Current capacity
//	int max_capacity;					// Maximum capacity
//	Point coordinates;					// 0 ~ 1000
//	unsigned int airport_start;
//	unsigned int airport_end;
//} Airplane;
//
//typedef struct passenger {
//	unsigned int id;					// 191 ~ 1190
//	unsigned int active : 1;
//	TCHAR name[MAX_NAME];
//	int wait_time;
//	unsigned int airport;
//	unsigned int airport_end;
//	unsigned int airplane;				// 0 not in airplane, 91 ~ 190 in an airplane
//} Passenger;

//typedef union command {
//	Point coordinates;
//	TCHAR str[MAX_NAME];
//} Command;
//
//typedef struct sharedbuffer {
//	unsigned int id;					//TODO Maybe change to PID...
//	unsigned int cmd_id;
//	Command command;
//} SharedBuffer;
//
//typedef struct sharedmemory {
//	unsigned int map[MAX_MAP][MAX_MAP];	// IDs (0 = empty, 1 ~ 90 = airports, 91 ~ 190 = airplanes)
//	BOOL accepting_planes;
//	int inC, outC;
//	SharedBuffer bufferControl[MAX_SHARED_BUFFER];
//	int inA, outA;
//	SharedBuffer bufferAirplane[MAX_SHARED_BUFFER];
//} SharedMemory;

typedef struct cfg {
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
	HANDLE obj_map;
	HANDLE mtx_instance;				// Mutex used to verify various instances of the same program
	HANDLE mtx_memory;					// Mutex for shared memory access
	HANDLE mtx_airport;					// Mutex for airport access
	HANDLE mtx_airplane;				// Mutex for airplane access
	HANDLE mtx_passenger;				// Mutex for passenger access
	HANDLE stop_event;					// Kill event (Stops the whole system)
} Config;

int init_config(Config *);
void end_config(Config *);
void init_control(Config *);

DWORD WINAPI read_command(void *);
DWORD WINAPI read_shared_memory(void *);
DWORD WINAPI read_named_pipes(void *);

void *get_by_id(Config *, unsigned int);
Airport *get_airport_by_id(Config *, unsigned int);
Airplane *get_airplane_by_id(Config *, unsigned int);
Passenger *get_passenger_by_id(Config *, unsigned int);
Airport *get_available_airport(Config *);
Airplane *get_available_airplane(Config *);
Passenger *get_available_passenger(Config *);
Airport *get_airport_by_name(Config *, const TCHAR *);
Airplane *get_airplane_by_name(Config *, const TCHAR *);
Passenger *get_passenger_by_name(Config *, const TCHAR *);

BOOL add_airport(Config *, Airport *);
BOOL add_airplane(Config *, Airplane *);
BOOL add_passenger(Config *, Passenger *);
BOOL remove_airport(Config *, unsigned int);
BOOL remove_airplane(Config *, unsigned int);
BOOL remove_passenger(Config *, unsigned int);

void print_airports(Config *);
void print_airplane(Config *);
void print_passenger(Config *);

#endif // !CONTROL_H
