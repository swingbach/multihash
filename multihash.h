#ifndef __MULTIHASH_H__
#define __MULTIHASH_H__

typedef struct 
{
	void               *mem;

	unsigned           *prime;	
	unsigned            rows;
	unsigned            cols;
	unsigned            cell_size;
} mul_hashtable_t;

mul_hashtable_t *mul_hashtable_create(unsigned rows, unsigned cols, unsigned cell_size);

char *mul_hashtable_lookup(mul_hashtable_t *table, char *key, unsigned *times);

int mul_hashtable_insert(mul_hashtable_t *table, char *key, char *value);
#endif
