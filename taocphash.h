#ifndef __TAOCPHASH_H__
#define __TAOCPHASH_H__

typedef struct 
{
	void               *mem;
	
	unsigned            total;
	unsigned            cell_size;
} tc_hashtable_t;

tc_hashtable_t *tc_hashtable_create(unsigned total, unsigned cell_size);

char *tc_hashtable_lookup(tc_hashtable_t *table, char *key, unsigned *times);

int tc_hashtable_insert(tc_hashtable_t *table, char *key, char *value);
#endif
