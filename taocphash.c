#include <stdlib.h>
#include <sys/shm.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "taocphash.h"

static unsigned tc_hash(const unsigned char *buf);
static int tc_is_prime(unsigned num);
static unsigned tc_find_max_prime_lt(unsigned num);

tc_hashtable_t *
tc_hashtable_create(unsigned total, unsigned cell_size)
{
	
	int n = 0;

	tc_hashtable_t *table = calloc(1, sizeof(tc_hashtable_t));
	assert(table);

	table->total     = total;
	table->cell_size = cell_size;

	table->mem = calloc(1, table->total * cell_size);
	assert(table->mem);

	return table;
}

/**
 *  return 1 if cell in given slot is empty or the key 
 *  return 0 if something else
 */
static int tc_find_hash_slot(tc_hashtable_t *table, unsigned slot, char **cell, char *key)
{
	int found = 0;
	char *tmp = NULL;

	*cell = NULL;

	tmp = (unsigned char *)table->mem + slot * table->cell_size;

	if (*tmp == 0) {
		found = 1;
		goto ret;
	}
	
	if (strcmp(tmp, key) == 0) {
		found = 1;
		*cell = tmp;
	}

ret:
	return found;
}

static void tc_set_hash_slot(tc_hashtable_t *table, unsigned slot, char *key, char *value)
{
	int klen, vlen;
	char *tmp;

	tmp = (unsigned char *)table->mem + slot * table->cell_size;

	assert(*tmp == 0 || strcmp(tmp, key) == 0);

	klen = strlen(key);
	vlen = strlen(value);

	assert(klen < 16);
	assert(vlen < 16);

	memcpy(tmp, key, klen);
	tmp[klen] = '\0';

	memcpy(tmp + klen + 1, value, vlen);

}

char *tc_hashtable_lookup(tc_hashtable_t *table, char *key, unsigned *times)
{
	long r, q, hs;
	unsigned long hash, s;
	char *cell = NULL;

	*times = 1;
	hash = tc_hash(key);

	//h1(key)
	r = hash % table->total;
	
	if (tc_find_hash_slot(table, r, &cell, key))
		goto ret;

	// h2(key)
    q = hash % (table->total - 2) + 1;
    for (s = 1; s < table->total; ++s) {
        hs = labs((r + s * q) % table->total);
        if (hs < 0)
            continue;

		(*times)++;

		if (tc_find_hash_slot(table, hs, &cell, key))
			break;
    }

ret:
	return cell;
}

int tc_hashtable_insert(tc_hashtable_t *table, char *key, char *value)
{
	int ret = -1;
	long r, q, hs;
	unsigned long hash, s;
	char *cell = NULL;

	hash = tc_hash(key);

	//h1(key)
	r = hash % table->total;
	
	if (tc_find_hash_slot(table, r, &cell, key)) {
		tc_set_hash_slot(table, r, key, value);
		ret = 0;
		goto exit;
	}

	// h2(key)
    q = hash % (table->total - 2) + 1;
    for (s = 1; s < table->total; ++s) {
        hs = labs((r + s * q) % table->total);
        if (hs < 0)
            continue;

		if (tc_find_hash_slot(table, hs, &cell, key)) {
			tc_set_hash_slot(table, hs, key, value);
			ret = 0;
			break;
		}
	
    }

exit:
	return ret;
}

static unsigned tc_find_max_prime_lt(unsigned num)
{
	unsigned prime = 1;

	for (num = num - 1; num > 0; num--) {
		if (tc_is_prime(num)) {
			prime = num;
			break;
		}
	}

	return prime;
}

static int tc_is_prime(unsigned num)
{
	int i;
	int prime = 1;

	for (i = num/2; i > 1; i--) {
		if (num % i == 0) {
			prime = 0;
			break;
		}
	}

	return prime;
}

static unsigned tc_hash(const unsigned char *buf) {
    unsigned int hash = 5381;

    while (*buf)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;
}

static void usage()
{
	fprintf(stdout, "usage: ./tchash <totals> <cell_size> <fill_factor>\n");
	exit(1);
}

int main(int argc, char **argv)
{
	unsigned int *stats;
	unsigned times, n;
	tc_hashtable_t *table;
	unsigned totals, cell_size, fill_factor;
	unsigned long cells_total, cells_fill_total;
	char *lines;

	if (argc != 4) 
		usage();
	
	totals      = atoi(argv[1]);
	cell_size   = atoi(argv[2]);
	fill_factor = atoi(argv[3]);

	cells_total = tc_find_max_prime_lt(totals);
	cells_fill_total = (fill_factor * cells_total) / 100;

	printf("prepare to fill table, cells = %lu, cells fill = %lu\n\n", cells_total, cells_fill_total);

	table = tc_hashtable_create(cells_total, cell_size);
	assert(table);

	stats = calloc(30, sizeof(unsigned int));

	//fill hashtable
	srand(100);
	for (n = 0; n < cells_fill_total; n++) {
		int len;
		char key[32];
		len = snprintf(key, 32, "%d\0", rand());
		assert(len < 32); // assert no truncate happens

		if (tc_hashtable_insert(table, key, "v") != 0) {
			fprintf(stdout, "failed to find a slot to insert n = %d\n", n);
			exit(1);
		}
	}

	//lookup data,stats times
	srand(100);
	for (n = 0; n < cells_fill_total; n++) {
		int len;
		unsigned times;
		char key[32];
		len = snprintf(key, 32, "%d\0", rand());
		assert(len < 32); // assert no truncate happens

		lines = tc_hashtable_lookup(table, key, &times);
		assert(strcmp(lines, key) == 0);

		if (times > 30)
			times = 30;

		stats[times - 1]++;
	}

	//output stats
	printf("=============output stats===============\n");
	for (n = 0; n < 29; n++) {
		printf("lookup times = %d : count = %d, hit %.1f%%\n", n + 1, stats[n], (stats[n] / (float)cells_fill_total) * 100);
	}
	printf("lookup times > %d : count = %d, hit %.1f%%\n", n + 1, stats[n], (stats[n] / (float)cells_fill_total) * 100);

}
