#pragma once

typedef struct hs_Entry {
	char* key;
	void* value;
	struct hs_Entry* next;
} hs_Entry;

typedef struct HashTable {
	hs_Entry** buckets;
	int size;
	int count;
} HashTable;

unsigned int hash(const char* key, int size);

HashTable* hs_create(int initial_size);
void* hs_get(HashTable* table, const char* key);
void hs_resize(HashTable* table);
void hs_insert(HashTable* table, const char* key, void* value);
void hs_free(HashTable* table);