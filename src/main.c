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
			break;

		case star_class_b:
			s.radius      = myrandf_uniform(3, 7);
			s.mass        = s.radius * myrandf_uniform(1.9f, 2.1f);
			s.luminosity  = s.radius * myrandf_uniform(9000.0f, 11000.0f);
			s.temperature = myrandi_uniform(100, 300) * 100;
			break;

		case star_class_a:
			s.radius      = myrandf_uniform(1.5f, 2.0f);
			s.mass        = s.radius * myrandf_uniform(0.8f, 1.2f);
			s.luminosity  = s.radius * myrandf_uniform(15.0f, 25.0f);
			s.temperature = myrandi_uniform(76, 100) * 100;
			break;

		case star_class_f:
			s.radius      = myrandf_uniform(1.0f, 1.4f);
			s.mass        = s.radius * myrandf_uniform(0.8f, 1.2f);
			s.luminosity  = s.radius * myrandf_uniform(3.0f, 3.5f);
			s.temperature = myrandi_uniform(60, 76) * 100;
			break;

		case star_class_g:
			s.radius      = myrandf_uniform(0.8f, 1.2f);
			s.mass        = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = myrandi_uniform(53, 60) * 100;
			break;

		case star_class_k:
			s.radius      = myrandf_uniform(0.6f, 0.9f);
			s.mass        = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = myrandi_uniform(39, 52) * 100;
			break;

		case star_class_m_giant:
			s.radius      = myrandf_uniform(10, 50);
			s.mass        = myrandf_uniform(0.3f, 8.0f);
			s.luminosity  = myrandf_uniform(50, 1000);
			s.temperature = myrandi_uniform(30, 100) * 100;
			break;

		case star_class_m_dwarf:
			s.radius      = myrandf_uniform(0.1f, 0.5f);
			s.mass        = s.radius * myrandf_uniform(0.8f, 1.0f);
			s.luminosity  = s.radius * myrandf_uniform(0.1f, 0.2f);
			s.temperature = myrandi_uniform(23, 38) * 100;
			break;

		case star_class_d:
			s.radius      = myrandf_uniform(0.008f, 0.02f);
			s.mass        = myrandf_uniform(0.5f, 0.7f);
			s.luminosity  = s.radius * myrandf_uniform(0.9f, 1.1f);
			s.temperature = s.luminosity * 1000000; // 8k-20k
			break;
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

	for(int i = 0; i < 32; i++) {
		system_t s = create_system();
		printf("System '%s' at %d, %d with a %s star at %d degrees\n",
				s.name, s.coord.x, s.coord.y,
				star_class_to_string(s.star.class), s.star.temperature);
	}
	return 0;
}

