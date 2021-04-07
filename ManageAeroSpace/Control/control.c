#include "control.h"

int init_config(Config *cfg) {
	memset(cfg, 0, sizeof(Config));

	// Get MAX_... from registry
	cfg->MAX_AIRPORT = 90;
	cfg->MAX_AIRPLANE = 100;
	cfg->MAX_PASSENGER = 1000;

	cfg->airports = calloc(cfg->MAX_AIRPORT, sizeof(Airport));
	cfg->airplanes = calloc(cfg->MAX_AIRPLANE, sizeof(Airplane));
	cfg->passengers = calloc(cfg->MAX_PASSENGER, sizeof(Passenger));

	if (cfg->airports == NULL || cfg->airplanes == NULL || cfg->passengers == NULL) {
		end_config(cfg);
		return FALSE;
	}

	int index = 1;
	for (int i = 0; i < cfg->MAX_AIRPORT; i++) {
		cfg->airports[i].id = (index++);
	}
	for (int i = 0; i < cfg->MAX_AIRPLANE; i++) {
		cfg->airplanes[i].id = (index++);
	}
	for (int i = 0; i < cfg->MAX_PASSENGER; i++) {
		cfg->passengers[i].id = (index++);
	}

	return TRUE;
}

void end_config(Config *cfg) {
	free(cfg->airports);
	free(cfg->airplanes);
	free(cfg->passengers);
	memset(cfg, 0, sizeof(Config));
}

void init_control(Config *cfg) {
	// init window (Win32)
}

void *get_by_id(Config *cfg, int id) {
	if (id < 1 || id >(cfg->MAX_AIRPORT + cfg->MAX_AIRPLANE + cfg->MAX_PASSENGER))
		return NULL;

	if (id <= cfg->MAX_AIRPORT) {
		// Airport
		return &(cfg->airports[id - 1]);
	} else if (id <= (cfg->MAX_AIRPORT + cfg->MAX_AIRPLANE)) {
		// Airplane
		return &(cfg->airplanes[id - (cfg->MAX_AIRPORT + 1)]);
	} else {
		// Passenger
		return &(cfg->passengers[id - (cfg->MAX_AIRPORT + cfg->MAX_AIRPLANE + 1)]);
	}
}

Airport *get_airport_by_id(Config *cfg, int id) {
	if (id > cfg->MAX_AIRPORT)
		return NULL;

	return get_by_id(cfg, id);
}

Airplane *get_airplane_by_id(Config *cfg, int id) {
	if (id < cfg->MAX_AIRPORT + 1 || id >(cfg->MAX_AIRPORT + cfg->MAX_AIRPLANE))
		return NULL;

	return get_by_id(cfg, id);
}

Passenger *get_passenger_by_id(Config *cfg, int id) {
	if (id < (cfg->MAX_AIRPORT + cfg->MAX_AIRPLANE + 1))
		return NULL;

	return get_by_id(cfg, id);
}
