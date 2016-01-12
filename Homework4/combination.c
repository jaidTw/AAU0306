#include "combination.h"
#include <stdlib.h>

void comb_gen_init(comb_gen* gen, int n, int k) {
	gen->n = n;
	gen->k = k;
	gen->comb = NULL;
}

int *comb_gen_next(comb_gen *gen) {
	if(gen->comb == NULL) {
		gen->comb = calloc(gen->k, sizeof(int));
		int i;
		for(i = 1; i <= gen->k; ++i)
			gen->comb[i - 1] = i;
		return gen->comb;
	}
	if(gen->comb[0] == gen->n - gen->k + 1) {
		free(gen->comb);
		gen->comb = NULL;
		return gen->comb;
	}
	int pos = gen->k - 1;
	while(gen->comb[pos] == gen->n - gen->k + 1 + pos) {
		--pos;
	}
	++gen->comb[pos];
	if(pos != gen->k - 1) {
		++pos;
		while(pos < gen->k) {
			gen->comb[pos] = gen->comb[pos - 1] + 1;
			++pos;
		}
	}
	return gen->comb;
}

void comb_gen_clear(comb_gen *gen) {
	free(gen->comb);
	gen->comb = NULL;
}