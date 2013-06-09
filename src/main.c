#include <stdio.h>

#include "random.h"
#include "namegen.h"
#include "space.h"
#include "politics.h"

#define NUM_NATIONS			4

void write_satellite_info(const satellite* s, int temperature, char buf[static 256])
{
	snprintf(buf, 255, "%5.2f AU, %5d degrees, %5.2f earth masses", 
			s->orbit.semimajor_axis,
			temperature - 273,
			s->mass);
	buf[255] = 0;
}

int main(void)
{
	mysrand(21);
	if(initialise_name_generation()) {
		return 1;
	}

	system_group systems;

	create_system_group(&systems);

	nation nations[NUM_NATIONS];
	settlement_group settlements;
	settlements.num_settlements = 0;

	for(int i = 0; i < NUM_NATIONS; i++) {
		nations[i] = create_nation(i);
	}

	create_settlement_collection(NUM_NATIONS, &systems, &settlements);

	for(int i = 0; i < systems.num_systems; i++) {
		system_t* s = &systems.systems[i];
		printf("System %d: '%s' at %d, %d with a %s star at %d degrees\n",
				i + 1, s->name, s->coord.x, s->coord.y,
				star_class_to_string(s->star.class), s->star.temperature);
		for(int j = 0; j < s->star.num_planets; j++) {
			planet* p = &s->star.planets[j];
			char planet_info[256];
			write_satellite_info(&p->planet,
					satellite_temperature(&p->planet, NULL, &s->star),
					planet_info);
			printf("\tPlanet %d: %-50s (%s)\n",
					j + 1, satellite_description(&p->planet),
					planet_info);
			for(int k = 0; k < p->num_moons; k++) {
				char moon_info[256];
				const satellite* moon = &p->moons[k];
				write_satellite_info(moon,
						satellite_temperature(moon, &p->planet, &s->star),
						moon_info);
				printf("\t\tMoon %d: %-44s (%s)\n",
						k + 1, satellite_description(&p->moons[k]),
						moon_info);
			}
		}
	}

	for(int i = 0; i < settlements.num_settlements; i++) {
		locator* l = &settlements.settlements[i].locator;
		printf("Settlement %d at system %d, planet %d", i + 1,
				l->system + 1,
				l->planet + 1);
		if(l->moon != 0xff)
			printf(", moon %d\n", l->moon + 1);
		else
			printf("\n");
	}

	return 0;
}

