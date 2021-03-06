#include <stdlib.h>
#include <sys/shm.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "multihash.h"

static unsigned mul_hash(const unsigned char *buf);
static int mul_is_prime(unsigned num);
static unsigned mul_find_max_prime_lt(unsigned num);

mul_hashtable_t *
mul_hashtable_create(unsigned rows, unsigned cols, unsigned cell_size)
{
	
	int n = 0;
	unsigned lprime;

	mul_hashtable_t *table = calloc(1, sizeof(mul_hashtable_t));
	assert(table);

	table->rows      = rows;
	table->cols      = cols;
	table->cell_size = cell_size;

	table->mem = calloc(1, rows * cols * cell_size);
	assert(table->mem);

	table->prime = calloc(rows, sizeof(unsigned));
	assert(table->prime);

	lprime = table->cols + 1;
	for (n = 0; n < table->rows; n++) {		
		table->prime[n] = mul_find_max_prime_lt(lprime);
		lprime = table->prime[n];
	}

	
	return table;
}

char *mul_hashtable_lookup(mul_hashtable_t *table, char *key, unsigned *times)
{
	int n;
	char *cell, *r = NULL;
	unsigned prime, hash;
	size_t roff, coff;

	hash = mul_hash(key);

	for (n = 0; n < table->rows; n++) {
		prime = table->prime[n];
		
		roff = n * (table->cols * table->cell_size);
		coff = (hash % prime) * table->cell_size;

		cell = ((unsigned char *)table->mem + roff + coff);
		
		if (*cell == 0)
			break;
		
		if (strcmp(cell, key) == 0) {
			r = cell;
			break;
		}
	}

	*times = n + 1;

	return r;
}

int mul_hashtable_insert(mul_hashtable_t *table, char *key, char *value)
{
	int n, ret = -1;
	size_t roff, coff;
	unsigned prime, hash;
	char *cell = NULL, *tmp;
	
	hash = mul_hash(key);

	for (n = 0; n < table->rows; n++) {
		prime = table->prime[n];
		
		roff = n * (table->cols * table->cell_size);
		coff = (hash % prime) * table->cell_size;

		tmp = ((unsigned char *)table->mem + roff + coff);
		
		if (*tmp == 0 || strcmp(tmp, key) == 0) {
			cell = tmp;
			break;
		}
	}

	if (cell) {
		int klen, vlen;

		klen = strlen(key);
		vlen = strlen(value);
		
		memcpy(cell, key, klen);
		cell[klen] = '\0';
		memcpy(cell + klen + 1, value, vlen);

		ret = 0;
	}

	return ret;
}

static unsigned mul_find_max_prime_lt(unsigned num)
{
	unsigned prime = 1;

	for (num = num - 1; num > 0; num--) {
		if (mul_is_prime(num)) {
			prime = num;
			break;
		}
	}

	return prime;
}

static int mul_is_prime(unsigned num)
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

static unsigned mul_hash(const unsigned char *buf) {
    unsigned int hash = 5381;

    while (*buf)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;
}

static void usage()
{
	fprintf(stdout, "usage: ./multihash <rows> <cols> <cell_size> <fill_factor>\n");
	exit(1);
}

int main(int argc, char **argv)
{
	unsigned int *stats;
	unsigned times, n;
	mul_hashtable_t *table;
	unsigned rows, cols, cell_size, fill_factor;
	unsigned long cells_total, cells_fill_total;
	char *lines;

	if (argc != 5) 
		usage();
	
	rows        = atoi(argv[1]);
	cols        = atoi(argv[2]);
	cell_size   = atoi(argv[3]);
	fill_factor = atoi(argv[4]);

	cells_total = rows * cols;
	cells_fill_total = (fill_factor * cells_total) / 100;

	printf("prepare to fill table, cells = %lu, cells fill = %lu\n\n", cells_total, cells_fill_total);

	table = mul_hashtable_create(rows, cols, cell_size);
	assert(table);

	stats = calloc(15, sizeof(unsigned int));

	//fill hashtable
	srand(100);
	for (n = 0; n < cells_fill_total; n++) {
		int len;
		char key[32];
		len = snprintf(key, 32, "%d\0", rand());
		assert(len < 32); // assert no truncate happens

		assert(mul_hashtable_insert(table, key, "v") == 0);
	}

	//lookup data,stats times
	srand(100);
	for (n = 0; n < cells_fill_total; n++) {
		int len;
		unsigned times;
		char key[32];
		len = snprintf(key, 32, "%d\0", rand());
		assert(len < 32); // assert no truncate happens

		lines = mul_hashtable_lookup(table, key, &times);
		assert(strcmp(lines, key) == 0);

		if (times > 15)
			times = 15;

		stats[times - 1]++;
	}

	//output stats
	printf("=============output stats===============\n");
	for (n = 0; n < 14; n++) {
		printf("lookup times = %d : count = %d, hit %.1f%%\n", n + 1, stats[n], (stats[n] / (float)cells_fill_total) * 100);
	}
	printf("lookup times > %d : count = %d, hit %.1f%%\n", n + 1, stats[n], (stats[n] / (float)cells_fill_total) * 100);

}
