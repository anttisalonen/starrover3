#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>

#define SECTOR_SIDE	256

typedef unsigned char byte;

/* random functions */

void mysrand(uint32_t s)
{
	srand(s);
}

uint32_t myrand(void)
{
	return rand();
}

inline uint32_t myrandi(int i);
uint32_t myrandi(int i)
{
	assert(i != 0);
	return myrand() % i;
}

inline int myrandi_uniform(int a, int b);
int myrandi_uniform(int a, int b)
{
	assert(b > a);
	assert(a != b);
	return (myrand() % (b - a)) + a;
}

inline float myrandf_uniform(float a, float b);
float myrandf_uniform(float a, float b)
{
	assert(b > a);
	int r = myrand();
	return a + (r / (float)INT_MAX) * (b - a);
}

byte myrandbyte(void)
{
	return myrand() & 0xff;
}

/* Markov list */
static uint32_t letter_table[28][28];

void add_name_to_table(const char* name)
{
	// http://www.gamasutra.com/view/feature/131784/algorithms_for_an_infinite_universe.php?page=3
	int len = strlen(name);
	int first = name[0] - 'a' + 1;
	int last = name[len - 1] - 'a' + 1;
	assert(islower(name[0]));
	assert(islower(name[len - 1]));
	letter_table[0][first]++; // space followed by letter
	letter_table[last][27]++; // letter followed by space

	int i;
	for(i = 0; i < len - 2; i++) {
		assert(islower(name[i]));
		assert(islower(name[i + 1]));
		first = name[i] - 'a' + 1;
		last = name[i + 1] - 'a' + 1;
		letter_table[first][last]++;
	}
}

int initialise_name_generation(void)
{
	memset(letter_table, 0x00, sizeof(letter_table));
#if 0
	add_name_to_table("oliver");
	add_name_to_table("henry");
	add_name_to_table("lucas");
	add_name_to_table("william");
	add_name_to_table("alexander");
	add_name_to_table("gabriel");
	add_name_to_table("james");
	add_name_to_table("samuel");
	add_name_to_table("luke");
	add_name_to_table("andrew");

	add_name_to_table("charlotte");
	add_name_to_table("sofia");
	add_name_to_table("scarlett");
	add_name_to_table("aurora");
	add_name_to_table("stella");
	add_name_to_table("juliette");
	add_name_to_table("isabella");
	add_name_to_table("hazel");
	add_name_to_table("natalie");
	add_name_to_table("penelope");

	add_name_to_table("quinn");
	add_name_to_table("willow");
	add_name_to_table("keira");
	add_name_to_table("victoria");
	add_name_to_table("molly");
	add_name_to_table("daniel");
	add_name_to_table("felix");
	add_name_to_table("josephine");
	add_name_to_table("elizabeth");
	add_name_to_table("max");
	add_name_to_table("xavier");
	add_name_to_table("charles");
	add_name_to_table("eva");
	add_name_to_table("blake");
	add_name_to_table("bennett");
	add_name_to_table("anna");
	add_name_to_table("ian");
	add_name_to_table("graham");
	add_name_to_table("gemma");
#else
	FILE* f = fopen("share/great_expectations_chapter_xv.txt", "r");
	if(!f) {
		fprintf(stderr, "Can't open text source!\n");
		return 1;
	}
	char word[64];
	int word_pos = 0;
	memset(word, 0x00, sizeof(word));
	size_t ret;
	char buf[1024];
	while(1) {
		ret = fread(buf, 1, 1024, f);
		if(ret <= 0)
			break;
		for(int i = 0; i < ret; i++) {
			if(word_pos == 63 || (word_pos > 0 && !isalpha(buf[i]))) {
				add_name_to_table(word);
				word_pos = 0;
				memset(word, 0x00, sizeof(word));
			} else if(isalpha(buf[i])) {
				word[word_pos++] = tolower(buf[i]);
			}
		}
	}
	fclose(f);
#endif

#if 0
	printf("    ");
	for(int i = 0; i < 27; i++) {
		printf("%c   ", i + 'a' - 1);
	}
	printf("\n");

	for(int i = 0; i < 27; i++) {
		if(i == 0)
			printf("    ");
		else
			printf("%c   ", i + 'a' - 1);
		for(int j = 0; j < 27; j++) {
			printf("%-3d ", letter_table[i][j]); 
		}
		printf("\n");
	}
#endif

	return 0;
}

int get_letter_position(int prev)
{
	int freq_total = 0;
	int i;
	for(i = 1; i < 26; i++) {
		freq_total += letter_table[prev][i];
	}

	assert(freq_total > 0);

	int rand_letter = myrand() % freq_total;

	freq_total = 0;
	for(i = 0; freq_total < rand_letter; i++) {
		freq_total += letter_table[prev][i];
	}
	return i - 1;
}

void get_random_name(int num_letters, char* name)
{
	int pos = 0;
	int i;
	for(i = 0; i < num_letters; i++) {
		pos = get_letter_position(pos);
		if(pos == -1)
			break;
		*name++ = pos + 'a' - 1;
	}
}

/* space stuff */
#define MAX_PLANETS_AROUND_STAR 8

typedef enum planet_surface {
	planet_surface_rock,
	planet_surface_gas,
} planet_surface;

typedef enum planet_atmosphere {
	planet_atmosphere_none,
	planet_atmosphere_co2,
	planet_atmosphere_oxygen,
	planet_atmosphere_hydrogen,
	planet_atmosphere_nitrogen, // must be the last one
} planet_atmosphere;

typedef struct planet {
	float radius; // unit: earth radius
	float mass; // unit: earth mass
	planet_surface surface;
	planet_atmosphere atmosphere;
	float atmospheric_pressure; // unit: kPa (100 kPa = earth)
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

const char* planet_description(const planet* p)
{
	assert(p);
	if(p->surface == planet_surface_gas) {
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
			case planet_atmosphere_none:
				return "rocky planetoid";
			case planet_atmosphere_co2:
				return "rocky planet with co2 atmosphere";
			case planet_atmosphere_oxygen:
				return "rocky planet with oxygen atmosphere";
			case planet_atmosphere_hydrogen:
				return "rocky planet with hydrogen atmosphere";
			case planet_atmosphere_nitrogen:
				return "rocky planet with nitrogen atmosphere";
		}
	}
	assert(0);
	return "";
}

planet create_planet(float expected_mass)
{
	planet p;
	assert(expected_mass > 0.001f);
	assert(expected_mass <= 100.0f);
	p.mass = myrandf_uniform(expected_mass * 0.1f, expected_mass * 10.0f);
	if(p.mass > 10.0f) {
		// gas
		p.surface = planet_surface_gas;
		// between 4 and 14
		p.radius = 4.0f * myrandf_uniform(0.9f, 1.1f) +
			(p.mass - 10.0f) * myrandf_uniform(0.05f, 0.1f);
		p.atmosphere = planet_atmosphere_hydrogen;
	} else {
		// rock
		p.surface = planet_surface_rock;
		// between 0.1 and 3.0
		if(p.mass < 2.0f)
			p.radius = 0.1f + p.mass * myrandf_uniform(0.8f, 1.2f);
		else
			p.radius = 2.0f + p.mass * 0.28f * myrandf_uniform(0.9f, 1.1f);
		if(p.mass < 0.01f) {
			p.atmosphere = planet_atmosphere_none;
		} else {
			int r = myrandi(planet_atmosphere_nitrogen - 1);
			r++;
			assert(r > planet_atmosphere_none);
			p.atmosphere = r;
			p.atmospheric_pressure = p.mass * 100.0f;
		}
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

star_class create_star_class(void)
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

star create_star(void)
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
			s.num_planets = myrandi_uniform(MAX_PLANETS_AROUND_STAR / 8, MAX_PLANETS_AROUND_STAR);
			break;

		case star_class_k:
			s.radius      = myrandf_uniform(0.6f, 0.9f);
			s.mass        = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = myrandi_uniform(39, 52) * 100;
			s.num_planets = myrandi_uniform(1, MAX_PLANETS_AROUND_STAR);
			break;

		case star_class_m_giant:
			s.radius      = myrandf_uniform(10, 50);
			s.mass        = myrandf_uniform(0.3f, 8.0f);
			s.luminosity  = myrandf_uniform(50, 1000);
			s.temperature = myrandi_uniform(30, 100) * 100;
			s.num_planets = myrandi_uniform(1, MAX_PLANETS_AROUND_STAR / 2);
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

	for(int i = 0; i < s.num_planets; i++) {
		float exp_mass = 10.0f * (i + 1) / (float)s.num_planets;
		s.planets[i] = create_planet(exp_mass);
	}

	return s;
}

system_coord create_system_coord(void)
{
	system_coord s;
	s.x = myrandbyte();
	s.y = myrandbyte();
	return s;
}

/* C99 6.7.5.3.7 (thanks Erlend) */
void create_system_name(char name[static 16])
{
	memset(name, 0x00, 16);
	int name_len = myrandi(10) + 4;
	get_random_name(name_len, name);
	name[0] = toupper(name[0]);
}

system_t create_system(void)
{
	system_t s;
	s.coord = create_system_coord();
	create_system_name(s.name);
	s.star = create_star();
	return s;
}

int main(void)
{
	mysrand(21);
	if(initialise_name_generation()) {
		return 1;
	}

	for(int i = 0; i < 8; i++) {
		system_t s = create_system();
		printf("System '%s' at %d, %d with a %s star at %d degrees\n",
				s.name, s.coord.x, s.coord.y,
				star_class_to_string(s.star.class), s.star.temperature);
		for(int j = 0; j < s.star.num_planets; j++) {
			printf("\tPlanet %d: %s (%3.2f earth masses)\n",
					j + 1, planet_description(&s.star.planets[j]),
					s.star.planets[j].mass);
		}
	}
	return 0;
}

