#ifndef __HW2_QUEUE
#define __HW2_QUEUE

#define QEMPTY 1
#define QFULL 2

typedef struct {
	int rear;
	int front;
	int max_size;
	int size;
	int* item;
} int_queue;

void int_queue_init(int_queue *Q, int size);
int int_queue_push(int_queue *Q, int val);
int int_queue_extract(int_queue *Q, int *val);
int int_queue_isempty(int_queue *Q);
void int_queue_print(int_queue *Q);
void int_queue_clear(int_queue *Q);

#endif