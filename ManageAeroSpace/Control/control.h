#ifndef CONTROL_H
#define CONTROL_H

#include <stdlib.h>
#include <Windows.h>
#include <tchar.h>

#define MAX_MAP 1000
#define MAX_NAME 50

#define MAX_AIRPORT 90
#define MAX_AIRPLANE 100
#define MAX_PASSENGER (100 * MAX_AIRPLANE)

typedef struct airport {
	unsigned int id;					// 1 ~ 90
	unsigned int active : 1;
	TCHAR name[MAX_NAME];
	unsigned int x, y;					// 0 ~ 1000
} Airport;

typedef struct airplane {
	unsigned int id;					// 91 ~ 190
	unsigned int active : 1;
	TCHAR name[MAX_NAME];
	int velocity;
	int capacity;						// Current capacity
	int max_capacity;					// Maximum capacity
	unsigned int x, y;					// 0 ~ 1000
	unsigned int airport_start;
	unsigned int airport_end;
} Airplane;

typedef struct passenger {
	unsigned int id;					// 191 ~ 1190
	unsigned int active : 1;
	TCHAR name[MAX_NAME];
	int wait_time;
	unsigned int airport;
	unsigned int airport_end;
	unsigned int airplane;				// 0 not in airplane, 91 ~ 190 in an airplane
} Passenger;

typedef struct cfg {
	int max_airport;					// Maximum number of airports
	int max_airplane;					// Maximum number of airplanes
	int max_passenger;					// Maximum number of passengers
	unsigned int next_airport_id;		// 1 <-> MAX_AIRPORT; ex: MAX_AIRPORT = 90
	unsigned int next_airplane_id;		// MAX_AIRPORT + 1 <-> (MAX_AIRPORT + MAX_AIRPLANE); ex: MAX_AIRPLANE = 100
	unsigned int next_passenger_id;		// (MAX_AIRPORT + MAX_AIRPLANE + 1) <-> (MAX_AIRPORT + MAX_AIRPLANE + MAX_PASSENGER); ex: MAX_PASSENGER = 100 * MAX_AIRPLANE
	unsigned int map[MAX_MAP][MAX_MAP];	// IDs (0 = empty, 1 ~ 90 = airports, 91 ~ 190 = airplanes)
	Airport *airports;					// List of airports
	Airplane *airplanes;				// List of airplanes
	Passenger *passengers;				// List of passengers
} Config;

int init_config(Config *cfg);
void end_config(Config *cfg);
void init_control(Config *cfg);

void *get_by_id(Config *cfg, unsigned int id);
Airport *get_airport_by_id(Config *cfg, unsigned int id);
Airplane *get_airplane_by_id(Config *cfg, unsigned int id);
Passenger *get_passenger_by_id(Config *cfg, unsigned int id);

#endif // !CONTROL_H
