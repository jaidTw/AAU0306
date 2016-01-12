#ifndef __HW2_COMB_GEN
#define __HW2_COMB_GEN

typedef struct {
	int n;
	int k;
	int* comb;
} comb_gen;

void comb_gen_init(comb_gen* gen, int n, int k);
int *comb_gen_next(comb_gen *gen);
void comb_gen_clear(comb_gen *gen);

#endif