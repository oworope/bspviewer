#include <stdlib.h>
#include <string.h>
#include <hashtable.h>

#ifdef _WIN32 // fix _strdup warning
#define strdup _strdup
#endif

// TODO: djb2
unsigned int hash(const char* key, int size) {
	unsigned int h = 0;
	while (*key) h = h * 31 + *key++;
    return h % size;
}

HashTable* hs_create(int initial_size) {
	HashTable* table = (HashTable*)malloc(sizeof(HashTable));
	table->size = initial_size;
	table->count = 0;
	table->buckets = (hs_Entry**)calloc(table->size, sizeof(hs_Entry*));
	return table;
}

void* hs_get(HashTable* table, const char* key) {
	unsigned int slot = hash(key, table->size);
	hs_Entry* entry = table->buckets[slot];

	while(entry != NULL) {
		if (strcmp(entry->key, key) == 0) {
			return entry->value;
		}
		entry = entry->next;
	}
	return NULL; // failed to find anything
}

void hs_resize(HashTable* table) {
	int old_size = table->size;
	int new_size = old_size * 2;
	hs_Entry** new_buckets = (hs_Entry**)calloc(new_size, sizeof(hs_Entry*));

	for (int i = 0; i < old_size; i++) {
		hs_Entry* entry = table->buckets[i];
		while (entry != NULL) {
			hs_Entry* next = entry->next;

			// recalculate new hash for new size
			unsigned int new_slot = hash(entry->key, new_size);

			// writing to new table
			entry->next = new_buckets[new_slot];
			new_buckets[new_slot] = entry;

			entry = next;
		}
	}

	free(table->buckets);
	table->buckets = new_buckets;
	table->size = new_size;
}

void hs_insert(HashTable* table, const char* key, void* value) {
	// recalc if size is 70%
	if ((float)table->count / (float)table->size > 0.7) {
		hs_resize(table);
	}

	unsigned int slot = hash(key, table->size);
	hs_Entry* entry = table->buckets[slot];

	while (entry != NULL) {
		if (strcmp(entry->key, key) == 0) {
			entry->value = value;
			return;
		}
		entry = entry->next;
	}

	hs_Entry* new_entry = malloc(sizeof(hs_Entry));
	new_entry->key = strdup(key);
	new_entry->value = value;
	new_entry->next = table->buckets[slot];
	table->buckets[slot] = new_entry;
	table->count++;
}

void hs_free(HashTable* table) {
	for (int i = 0; i < table->size; i++) {
		hs_Entry* entry = table->buckets[i];
		while (entry != NULL) {
			hs_Entry* tmp = entry;
			entry = entry->next;

			free(tmp->key);
			free(tmp);
		}
	}

	free(table->buckets);
	free(table);
}