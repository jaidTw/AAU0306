#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

void int_queue_init(int_queue *Q, int max_size) {
	Q->max_size = max_size;
	Q->size = 0;
	Q->item = calloc(max_size, sizeof(int));
	Q->front = 0;
	Q->rear = 0;
}

int int_queue_push(int_queue *Q, int val) {
	if((Q->rear + 1) % Q->max_size == Q->front)
		return QFULL;
	Q->item[Q->rear] = val;
	Q->rear = (Q->rear + 1) % Q->max_size;
	++Q->size;
	return 0;
}

int int_queue_extract(int_queue *Q, int *val) {
	if(Q->rear == Q->front)
		return QEMPTY;
	*val = Q->item[Q->front];
	Q->front = (Q->front + 1) % Q->max_size;
	--Q->size;
	return 0;
}

int int_queue_isempty(int_queue *Q) {
	return Q->size == 0;
}

void int_queue_print(int_queue *Q) {
	if(int_queue_isempty(Q)) {
 		//printf("{}\n");
		return;
	}
 	printf("{");
 	for(int i = Q->front; i != Q->rear; i = (i + 1) % Q->max_size)
 		printf("%d, ", Q->item[i]);
 	printf("\b\b}\n");
 }

void int_queue_clear(int_queue *Q) {
 	Q->max_size = 0;
 	Q->size = 0;
 	free(Q->item);
 	Q->item = NULL;
 	Q->front = 0;
 	Q->rear = 0;
 }
