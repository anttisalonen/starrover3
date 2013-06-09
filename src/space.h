#ifndef SR_SPACE_H
#define SR_SPACE_H

#define MAX_NUM_SYSTEMS_PER_SYSTEM_GROUP	32
#define MAX_PLANETS_AROUND_STAR			8
#define MAX_MOONS_AROUND_PLANET			8

typedef enum satellite_surface {
	satellite_surface_rock,
	satellite_surface_gas,
} satellite_surface;

typedef enum satellite_atmosphere {
	satellite_atmosphere_none,
	satellite_atmosphere_co2,
	satellite_atmosphere_oxygen,
	satellite_atmosphere_hydrogen,
	satellite_atmosphere_nitrogen, // must be the last one
} satellite_atmosphere;

typedef struct orbit {
	// for now assume eccentricity = 0, inclination = 0
	float semimajor_axis; // unit: au
	float avg_orbital_speed; // unit: km/s
} orbit;

typedef struct satellite {
	float radius; // unit: earth radius
	float mass; // unit: earth mass
	satellite_surface surface;
	satellite_atmosphere atmosphere;
	float atmospheric_pressure; // unit: 100 kPa (= pressure on earth)
	orbit orbit;
} satellite;

typedef struct planet {
	satellite planet;
	byte num_moons;
	satellite moons[MAX_MOONS_AROUND_PLANET];
} planet;

typedef enum star_class {
	star_class_o,
	star_class_b,
	star_class_a,
	star_class_f,
	star_class_g,
	star_class_k,
	star_class_m_dwarf,
	star_class_m_giant,
	star_class_d
} star_class;

typedef struct star {
	float radius; // unit: solar radius
	float mass;   // unit: solar mass
	float luminosity; // unit: solar luminosity
	uint32_t temperature; // unit: kelvin
	star_class class;
	byte num_planets;
	planet planets[MAX_PLANETS_AROUND_STAR];
} star;

typedef struct system_coord {
	byte x;
	byte y;
} system_coord;

typedef struct system_t {
	system_coord coord;
	char name[16];
	star star;
} system_t;

typedef struct system_group {
	byte num_systems;
	system_t systems[MAX_NUM_SYSTEMS_PER_SYSTEM_GROUP];
} system_group;

const char* star_class_to_string(star_class c);
const char* satellite_description(const satellite* p);

void create_system_group(system_group* s);

/* unit: kelvin */
int satellite_temperature(const satellite* sat, const satellite* primary, const star* star);

#endif

