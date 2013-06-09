#include <stdlib.h>
#include <assert.h>

#include "random.h"
#include "space.h"
#include "politics.h"

/* political stuff */

static const char* nation_names[MAX_NUM_NATIONS] = {
	"vulravian",
	"maugurian",
	"inderian",
	"andarian"
};

nation create_nation(int i)
{
	nation n;
	assert(i < MAX_NUM_NATIONS);
	n.name = nation_names[i];
	n.index = i;

	assert(MAX_NUM_NATIONS == 4);
	switch(i) {
		case 0: // fascist
			n.militaristic = 250;
			n.authoritarian = 250;
			n.capitalist = 250;
			break;

		case 1: // monks in a cave
			n.militaristic = 1;
			n.authoritarian = 250;
			n.capitalist = 20;
			break;

		case 2: // capitalist
			n.militaristic = 50;
			n.authoritarian = 190;
			n.capitalist = 250;
			break;

		case 3: // anarchist
			n.militaristic = 190;
			n.authoritarian = 30;
			n.capitalist = 20;
			break;
	}

	for(int k = 0; k < MAX_NUM_NATIONS; k++) {
		if(k == i)
			n.relationships[k] = relationship_peace;
		else
			n.relationships[k] = relationship_none;
	}

	return n;
}

/* adds a new settlements in settlements[settlements->num_settlements]. */
/* returns 1 if found and added, or 0 if not found. */
static int find_settlement(byte nation_index, const system_group* systems, settlement_group* settlements)
{
	assert(settlements);
	assert(systems);
	assert(settlements->num_settlements < MAX_NUM_SETTLEMENTS);

	for(int i = 0; i < 40; i++) {
		int sys_index = myrandi(systems->num_systems);
		const system_t* system = &systems->systems[sys_index];
		const star* star = &system->star;

		if(!star->num_planets)
			continue;
		int planet_index = myrandi(star->num_planets);
		const planet* planet = &star->planets[planet_index];
		const satellite* sat = &planet->planet;
		int moon_index = 0xff;
		if(planet->num_moons && sat->surface == satellite_surface_gas) {
			moon_index = myrandi(planet->num_moons);
			sat = &planet->moons[moon_index];
		}

		if(sat->surface == satellite_surface_gas)
			continue;

		if(sat->atmosphere != satellite_atmosphere_oxygen)
			continue;

		const satellite* primary = NULL;
		if(moon_index != 0xff)
			primary = &planet->planet;
		int temperature = satellite_temperature(sat, primary, star);
		if(temperature < 100 || temperature > 400)
			continue;

		if(sat->mass < 0.1f)
			continue;

		/* check if already populated */
		int populated = 0;
		for(int j = 0; j < settlements->num_settlements; j++) {
			if(settlements->settlements[j].locator.system != sys_index)
				continue;

			if(settlements->settlements[j].locator.star != 0)
				continue;

			if(settlements->settlements[j].locator.planet != planet_index)
				continue;

			if(settlements->settlements[j].locator.moon != moon_index)
				continue;

			populated = 1;
			break;
		}

		if(populated)
			continue;

		settlement* s = &settlements->settlements[settlements->num_settlements];
		s->locator.system = sys_index;
		s->locator.star   = 0;
		s->locator.planet = planet_index;
		s->locator.moon   = moon_index;
		s->nation_index   = nation_index;
		s->size = 1;
		s->wealth = 1;
		s->industrial = 0;
		s->agricultural = 0;
		return 1;
	}
	return 0;
}

void create_settlement_collection(byte num_nations, const system_group* systems, settlement_group* settlements)
{
	assert(num_nations > 0);
	for(int i = 0; i < MAX_NUM_SETTLEMENTS; i++) {
		int nation_id = i % num_nations;
		int found = find_settlement(nation_id, systems, settlements);
		if(!found)
			break;
		settlements->num_settlements = i;
	}

	return;
}


