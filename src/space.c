#include <assert.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "random.h"
#include "namegen.h"
#include "space.h"

/* space stuff */

int satellite_temperature(const satellite* sat, const satellite* primary, const star* star)
{
	assert(sat);
	assert(star);
	float tot_dist = sat->orbit.semimajor_axis;
	if(primary)
		tot_dist += primary->orbit.semimajor_axis;

	int star_temp = star->temperature;
	star_temp *= 0.15f;

	if(tot_dist < 0.001f)
		return star_temp;

	int temp = star_temp / powf(1.0f + tot_dist, 1.2f);

	if(sat->atmospheric_pressure < 1.0f) {
		if(sat->atmospheric_pressure > 0.4f)
			temp *= sat->atmospheric_pressure;
		else
			temp *= 0.4f;
	}

	if(sat->atmosphere == satellite_atmosphere_co2)
		temp = temp * 1.5f;

	return temp;
}

const char* satellite_description(const satellite* p)
{
	assert(p);
	if(p->surface == satellite_surface_gas) {
		if(p->mass < 20)
			return "small gas giant";
		else if(p->mass < 150)
			return "medium gas giant";
		else if(p->mass < 350)
			return "large gas giant";
		else
			return "huge gas giant";
	} else {
		switch(p->atmosphere) {
			case satellite_atmosphere_none:
				return "rocky planetoid";
			case satellite_atmosphere_co2:
				return "rocky planet with co2 atmosphere";
			case satellite_atmosphere_oxygen:
				return "rocky planet with oxygen atmosphere";
			case satellite_atmosphere_hydrogen:
				return "rocky planet with hydrogen atmosphere";
			case satellite_atmosphere_nitrogen:
				return "rocky planet with nitrogen atmosphere";
		}
	}
	assert(0);
	return "";
}

static satellite create_satellite(float expected_mass, const orbit* o)
{
	satellite p;
	assert(o);
	assert(expected_mass > 0.001f);
	assert(expected_mass <= 100.0f);
	p.mass = myrandf_uniform(expected_mass * 0.01f, expected_mass * 10.0f);
	if(p.mass > 10.0f) {
		// gas
		p.surface = satellite_surface_gas;
		// between 4 and 14
		p.radius = 4.0f * myrandf_uniform(0.9f, 1.1f) +
			(p.mass - 10.0f) * myrandf_uniform(0.05f, 0.1f);
		p.atmosphere = satellite_atmosphere_hydrogen;
		p.atmospheric_pressure = 1.0f;
	} else {
		// rock
		p.surface = satellite_surface_rock;
		// between 0.1 and 3.0
		if(p.mass < 2.0f)
			p.radius = 0.1f + p.mass * myrandf_uniform(0.8f, 1.2f);
		else
			p.radius = 2.0f + p.mass * 0.28f * myrandf_uniform(0.9f, 1.1f);
		if(p.mass < 0.01f) {
			p.atmosphere = satellite_atmosphere_none;
			p.atmospheric_pressure = 0.0f;
		} else {
			int r = myrandi(satellite_atmosphere_nitrogen - 1);
			r++;
			assert(r > satellite_atmosphere_none);
			p.atmosphere = r;
			p.atmospheric_pressure = p.mass;
		}
	}

	p.orbit = *o;

	return p;
}

static orbit create_planet_orbit(const star* star, int num)
{
	orbit o;
	assert(star);
	num++;
	o.semimajor_axis = star->radius * 0.1f + num * num * 0.2f * star->mass * myrandf_uniform(0.8f, 1.2f);
	o.avg_orbital_speed = star->mass * 100.0f / o.semimajor_axis;
	return o;
}

static orbit create_orbit(const satellite* primary, int num)
{
	orbit o;
	assert(primary);
	// calculate with earth radius as unit
	o.semimajor_axis = primary->radius * 3.0f + num * num * primary->mass * myrandf_uniform(0.8f, 1.2f);
	// convert to au
	o.semimajor_axis *= 0.000042634f;
	o.avg_orbital_speed = primary->mass * 10.0f / o.semimajor_axis;
	return o;
}

static planet create_planet(float expected_mass, const orbit* planet_orbit)
{
	planet p;
	assert(planet_orbit);
	p.planet = create_satellite(expected_mass, planet_orbit);

	p.num_moons = sqrt(p.planet.mass) * myrandf_uniform(0.5f, 2.0f);
	assert(p.num_moons >= 0);
	if(p.num_moons > MAX_MOONS_AROUND_PLANET)
		p.num_moons = MAX_MOONS_AROUND_PLANET;

	for(int i = 0; i < p.num_moons; i++) {
		float sat_exp_mass = 0.001f * p.planet.mass;
		if(p.planet.mass > 10.0f) {
			int rm = myrandi(10);
			if(rm == 0)
				sat_exp_mass *= myrandf_uniform(1.0f, 10.0f);
			else
				sat_exp_mass *= myrandf_uniform(0.001f, 1.0f);
		}
		if(sat_exp_mass > 0.01f * p.planet.mass)
			sat_exp_mass = 0.01f * p.planet.mass;

		if(sat_exp_mass > 1.0f)
			sat_exp_mass = 1.0f;
		if(sat_exp_mass < 0.0011f)
			sat_exp_mass = 0.0011f;

		orbit sat_orbit = create_orbit(&p.planet, i);
		p.moons[i] = create_satellite(sat_exp_mass, &sat_orbit);
	}
	return p;
}

const char* star_class_to_string(star_class c)
{
	switch(c) {
		case star_class_o:
			return "class O";

		case star_class_b:
			return "class B";

		case star_class_a:
			return "class A";

		case star_class_f:
			return "class F";

		case star_class_g:
			return "class G";

		case star_class_k:
			return "class K";

		case star_class_m_giant:
			return "red giant";

		case star_class_m_dwarf:
			return "red dwarf";

		case star_class_d:
			return "white dwarf";
	}
}

static star_class create_star_class(void)
{
	int r = myrandi(10000);
	if(r == 0) // really: 0.00003%
		return star_class_o;
	else if(r == 1)
		return star_class_b;
	else if(r < 62) // 0.625%
		return star_class_a;
	else if(r < 303) // 3.03%
		return star_class_f;
	else if(r < 1000) // 7.5%
		return star_class_g;
	else if(r < 2200) // 12%
		return star_class_k;
	else if(r < 2400) // 2% (don't know how realistic)
		return star_class_d;
	// according to Wikipedia, M is ~76% of all.
	// Split this to 75% dwarf, 1% giant.
	else if(r < 2500)
		return star_class_m_giant;
	else
		return star_class_m_dwarf;
}

static star create_star(void)
{
	star s;
	s.class = create_star_class();

	switch(s.class) {
		case star_class_o:
			s.radius      = myrandf_uniform(5, 15);
			s.mass        = s.radius * myrandf_uniform(4.8f, 5.2f);
			s.luminosity  = s.radius * myrandf_uniform(5900.0f, 6100.0f);
			s.temperature = myrandi_uniform(300, 520) * 100;
			s.num_planets = myrandi_uniform(MAX_PLANETS_AROUND_STAR / 2, MAX_PLANETS_AROUND_STAR + 1);
			break;

		case star_class_b:
			s.radius      = myrandf_uniform(3, 7);
			s.mass        = s.radius * myrandf_uniform(1.9f, 2.1f);
			s.luminosity  = s.radius * myrandf_uniform(9000.0f, 11000.0f);
			s.temperature = myrandi_uniform(100, 300) * 100;
			s.num_planets = myrandi_uniform(MAX_PLANETS_AROUND_STAR / 2, MAX_PLANETS_AROUND_STAR + 1);
			break;

		case star_class_a:
			s.radius      = myrandf_uniform(1.5f, 2.0f);
			s.mass        = s.radius * myrandf_uniform(0.8f, 1.2f);
			s.luminosity  = s.radius * myrandf_uniform(15.0f, 25.0f);
			s.temperature = myrandi_uniform(76, 100) * 100;
			s.num_planets = myrandi_uniform(MAX_PLANETS_AROUND_STAR / 2, MAX_PLANETS_AROUND_STAR + 1);
			break;

		case star_class_f:
			s.radius      = myrandf_uniform(1.0f, 1.4f);
			s.mass        = s.radius * myrandf_uniform(0.8f, 1.2f);
			s.luminosity  = s.radius * myrandf_uniform(3.0f, 3.5f);
			s.temperature = myrandi_uniform(60, 76) * 100;
			s.num_planets = myrandi_uniform(MAX_PLANETS_AROUND_STAR / 3, MAX_PLANETS_AROUND_STAR);
			break;

		case star_class_g:
			s.radius      = myrandf_uniform(0.8f, 1.2f);
			s.mass        = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = myrandi_uniform(53, 60) * 100;
			s.num_planets = myrandi_uniform(MAX_PLANETS_AROUND_STAR / 4, MAX_PLANETS_AROUND_STAR);
			break;

		case star_class_k:
			s.radius      = myrandf_uniform(0.6f, 0.9f);
			s.mass        = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = myrandi_uniform(39, 52) * 100;
			s.num_planets = myrandi_uniform(2, MAX_PLANETS_AROUND_STAR);
			break;

		case star_class_m_giant:
			s.radius      = myrandf_uniform(10, 50);
			s.mass        = myrandf_uniform(0.3f, 8.0f);
			s.luminosity  = myrandf_uniform(50, 1000);
			s.temperature = myrandi_uniform(30, 100) * 100;
			s.num_planets = myrandi_uniform(2, MAX_PLANETS_AROUND_STAR / 2);
			break;

		case star_class_m_dwarf:
			assert(MAX_PLANETS_AROUND_STAR >= 4);
			s.radius      = myrandf_uniform(0.1f, 0.5f);
			s.mass        = s.radius * myrandf_uniform(0.8f, 1.0f);
			s.luminosity  = s.radius * myrandf_uniform(0.1f, 0.2f);
			s.temperature = myrandi_uniform(23, 38) * 100;
			s.num_planets = myrandi_uniform(1, 4);
			break;

		case star_class_d:
			s.radius      = myrandf_uniform(0.008f, 0.02f);
			s.mass        = myrandf_uniform(0.5f, 0.7f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = s.luminosity * 1000000; // 8k-20k
			s.num_planets = myrandi_uniform(1, 4);
			break;
	}

	int orbit_num = 0;
	for(int i = 0; i < s.num_planets; i++) {
		orbit_num++;
		if(myrandi(3) == 0)
			orbit_num++;
		float exp_mass = 10.0f * (i + 1) / (float)s.num_planets;
		orbit planet_orbit = create_planet_orbit(&s, orbit_num);
		s.planets[i] = create_planet(exp_mass, &planet_orbit);
	}

	return s;
}

static system_coord create_system_coord(void)
{
	system_coord s;
	s.x = myrandbyte();
	s.y = myrandbyte();
	return s;
}

/* C99 6.7.5.3.7 (thanks Erlend) */
static void create_system_name(char name[static 16])
{
	memset(name, 0x00, 16);
	int name_len = myrandi(10) + 4;
	get_random_name(name_len, name);
	name[0] = toupper(name[0]);
}

static system_t create_system(void)
{
	system_t s;
	s.coord = create_system_coord();
	create_system_name(s.name);
	s.star = create_star();
	return s;
}

void create_system_group(system_group* s)
{
	assert(s);
	s->num_systems = MAX_NUM_SYSTEMS_PER_SYSTEM_GROUP;
	for(int i = 0; i < MAX_NUM_SYSTEMS_PER_SYSTEM_GROUP; i++) {
		s->systems[i] = create_system();
	}
}


