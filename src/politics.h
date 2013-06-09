#ifndef SR_POLITICS_H
#define SR_POLITICS_H

#define MAX_NUM_NATIONS		4
#define MAX_NUM_SETTLEMENTS	16

typedef enum relationship {
	relationship_none,
	relationship_peace,
	relationship_war,
	relationship_trade_embargo,
	relationship_alliance
} relationship;

typedef struct nation {
	const char* name;
	byte index;
	byte militaristic;  /* as opposed to pacifist */
	byte authoritarian; /* as opposed to libertanian */
	byte capitalist;    /* as opposed to planned economy */
	relationship relationships[MAX_NUM_NATIONS];
} nation;

typedef struct locator {
	byte system;
	byte star;
	byte planet;
	byte moon;
} locator;

typedef struct settlement {
	locator locator;
	byte nation_index;
	byte size;
	byte wealth;
	byte industrial;
	byte agricultural;
} settlement;

typedef struct settlement_group {
	int num_settlements;
	settlement settlements[MAX_NUM_SETTLEMENTS];
} settlement_group;

nation create_nation(int i);
void create_settlement_collection(byte num_nations, const system_group* systems, settlement_group* settlements);
const settlement* settlement_in(const locator* loc, const settlement_group* settlements);

#endif

