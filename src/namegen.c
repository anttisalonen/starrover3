#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "random.h"
#include "namegen.h"

/* Markov list */
static uint32_t letter_table[28][28];

static void add_name_to_table(const char* name)
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

static int get_letter_position(int prev)
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


