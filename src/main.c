#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

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

#if 1
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
typedef struct system_coord {
	byte x;
	byte y;
} system_coord;

typedef struct system_t {
	system_coord coord;
	char name[16];
} system_t;

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
	int name_len = myrand() % 10 + 4;
	get_random_name(name_len, name);
	name[0] = toupper(name[0]);
}

system_t create_system(void)
{
	system_t s;
	s.coord = create_system_coord();
	create_system_name(s.name);
	return s;
}

int main(void)
{
	mysrand(21);
	if(initialise_name_generation()) {
		return 1;
	}

	for(int i = 0; i < 100; i++) {
		system_t s = create_system();
		printf("System '%s' at %d, %d\n", s.name, s.coord.x, s.coord.y);
	}
	return 0;
}

