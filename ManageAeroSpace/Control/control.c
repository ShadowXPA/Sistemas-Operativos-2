#include "control.h"

int init_config(Config *cfg) {
	memset(cfg, 0, sizeof(Config));

	// Get MAX_... from registry
	cfg->max_airport = MAX_AIRPORT;
	cfg->max_airplane = MAX_AIRPLANE;
	cfg->max_passenger = MAX_PASSENGER;

	cfg->airports = calloc(cfg->max_airport, sizeof(Airport));
	cfg->airplanes = calloc(cfg->max_airplane, sizeof(Airplane));
	cfg->passengers = calloc(cfg->max_passenger, sizeof(Passenger));

	if (cfg->airports == NULL || cfg->airplanes == NULL || cfg->passengers == NULL) {
		end_config(cfg);
		return FALSE;
	}

	int index = 1;
	for (int i = 0; i < cfg->max_airport; i++) {
		cfg->airports[i].id = (index++);
	}
	for (int i = 0; i < cfg->max_airplane; i++) {
		cfg->airplanes[i].id = (index++);
	}
	for (int i = 0; i < cfg->max_passenger; i++) {
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
