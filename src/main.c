#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

typedef struct sector {
	settlement_group settlements;
	system_group systems;
} sector;

#define MAX_NUM_SECTORS 1

typedef struct star_group {
	int num_sectors;
	int num_nations;
	sector sectors[MAX_NUM_SECTORS];
	nation nations[NUM_NATIONS];
} star_group;

void create_star_group(star_group* sc)
{
	assert(sc);

	sc->num_sectors = MAX_NUM_SECTORS;
	for(int i = 0; i < sc->num_sectors; i++) {
		create_system_group(&sc->sectors[i].systems);
		sc->sectors[i].settlements.num_settlements = 0;
		create_settlement_collection(NUM_NATIONS, &sc->sectors[i].systems,
				&sc->sectors[i].settlements);
	}

	sc->num_nations = NUM_NATIONS;
	for(int i = 0; i < NUM_NATIONS; i++) {
		sc->nations[i] = create_nation(i);
	}
}

void print_complete_info(const star_group* sc)
{
	assert(sc);
	for(int l = 0; l < sc->num_sectors; l++) {
		const system_group* systems = &sc->sectors[l].systems;
		for(int i = 0; i < systems->num_systems; i++) {
			const system_t* s = &systems->systems[i];
			printf("System %d: '%s' at %d, %d with a %s star at %d degrees\n",
					i + 1, s->name, s->coord.x, s->coord.y,
					star_class_to_string(s->star.class), s->star.temperature);
			for(int j = 0; j < s->star.num_planets; j++) {
				const planet* p = &s->star.planets[j];
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

		const settlement_group* settlements = &sc->sectors[l].settlements;
		for(int i = 0; i < settlements->num_settlements; i++) {
			const locator* loc = &settlements->settlements[i].locator;
			printf("Settlement %d at system %d, planet %d", i + 1,
					loc->system + 1,
					loc->planet + 1);
			if(loc->moon != 0xff)
				printf(", moon %d\n", loc->moon + 1);
			else
				printf("\n");
		}
	}
}

typedef struct player_info {
	char name[32];
	byte difficulty_level;
	byte nationality;
	uint32_t money;
	unsigned int sector;
	locator location;
} player_info;

typedef struct game {
	star_group sc;
	player_info player;
} game;

void print_game_info(const game* game)
{
	assert(game);
	const player_info* p = &game->player;
	assert(p->sector < game->sc.num_sectors);

	printf("Player %s with %u gold.\n\n", p->name,
			p->money);

	const locator* loc = &p->location;
	const sector* sec = &game->sc.sectors[p->sector];
	assert(loc->system < sec->systems.num_systems);
	const system_t* sys = &sec->systems.systems[loc->system];
	assert(loc->planet < sys->star.num_planets);
	const planet* planet = &sys->star.planets[loc->planet];
	const satellite* sat;
	char sat_description[16];
	if(loc->moon != 0xff) {
		assert(loc->moon < planet->num_moons);
		sat = &planet->moons[loc->moon];
		sprintf(sat_description, "%d%c", loc->planet + 1,
				loc->moon + 'a');
	} else {
		sat = &planet->planet;
		sprintf(sat_description, "%d", loc->planet + 1);
	}
	const settlement* set = settlement_in(loc, &sec->settlements);
	byte owner = 0xff;
	if(set)
		owner = set->nation_index;
	printf("In system %s, planet %s, ", sys->name, sat_description);
	if(owner == 0xff) {
		printf("alone.\n");
	} else {
		printf("settlement controlled by the %ss.\n",
				game->sc.nations[owner].name);
	}
}

int main(void)
{
	mysrand(21);
	if(initialise_name_generation()) {
		return 1;
	}
	
	game game;
	create_star_group(&game.sc);

	printf("Size of the star cluster: %zd\n", sizeof(game.sc));

	memset(&game.player, 0x00, sizeof(game.player));
	const char* username = getenv("USER");
	if(username)
		strncpy(game.player.name, username, 31);
	else
		strcpy(game.player.name, "unknown hero");

	game.player.money = 1000;
	game.player.location = game.sc.sectors[0].settlements.settlements[0].locator;

	print_game_info(&game);

	return 0;
}

