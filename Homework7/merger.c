#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

#define ERR_EXIT(str, ...) { fprintf(stderr, str, ## __VA_ARGS__); exit(-1); }

int parsearg(char *);
void *sort(void *);
void *merge(void *);
int cmp(const void *, const void *);
static int *data;
int seg_size;

typedef struct {
    size_t start_pos;
    size_t end_pos;
} pos_inf;

int main(int argc, char **argv) {
    if(argc != 2) 
        ERR_EXIT("Usage : ./merger [segment_size]\n");
    seg_size = parsearg(argv[1]);
    
    int num;
    scanf("%d", &num);
    data = (int *)malloc(num * sizeof(int));
    if(data == NULL)
        ERR_EXIT("Memory allocation failed, please try again later.");

    for(int i = 0; i < num; ++i)
        scanf("%d", &data[i]);
    
    int seg_num = ceil((double)num / seg_size);
    pos_inf pos[seg_num];
    for(int i = 0; i < seg_num; ++i) {
        pos[i].start_pos = i * seg_size;
        pos[i].end_pos = (i + 1) * seg_size;
    }
    pos[seg_num - 1].end_pos = num;

    // sort
    pthread_t tid[seg_num];
    pthread_attr_t attr[seg_num];
    for(int i = 0; i < seg_num; ++i) {
        pthread_attr_init(&attr[i]);
        if(pthread_create(&tid[i], &attr[i], sort, (void *)&pos[i]))
            ERR_EXIT("An error occur while creating new thread.\n");
    }
    for(int i = 0; i < seg_num; ++i) {
        pthread_join(tid[i], NULL);
        pthread_attr_destroy(&attr[i]);
    }
    
    // merge
    while(ceil(seg_num / 2)) {
        int thread_num = floor((double)seg_num / 2);
        seg_num = ceil((double)seg_num / 2);
        seg_size *= 2;

        // compute segment interval
        pos_inf pos[thread_num][2];
        for(int i = 0; i < thread_num; ++i) {
            pos[i][0].start_pos = i * seg_size;
            pos[i][1].start_pos = pos[i][0].end_pos = i * seg_size + seg_size / 2;
            pos[i][1].end_pos = (i + 1) * seg_size;
        }
        if(seg_size * thread_num > num)
            pos[thread_num - 1][1].end_pos = num;

        for(int i = 0; i < thread_num; ++i) {
            pthread_attr_init(&attr[i]);
            if(pthread_create(&tid[i], &attr[i], merge, (void *)&pos[i]))
                ERR_EXIT("An error occur while creating new thread.\n");
        }
        
        for(int i = 0; i < thread_num; ++i) {
            pthread_join(tid[i], NULL);
            pthread_attr_destroy(&attr[i]);
        }
    }

    for(int i = 0; i < num; ++i)
        printf("%d ", data[i]);
    puts("");
    return 0;
}

int parsearg(char *str) {
    char *endptr;
    int seg_size = strtol(str, &endptr, 10);
    if(!(*str != '\0' && *endptr == '\0'))
        ERR_EXIT("Invalid argument. Usage : ./merger [segment_size]\n");
    return seg_size;
}

void *sort(void *pos) {
    size_t start_pos = ((pos_inf *)pos)->start_pos;
    size_t end_pos = ((pos_inf *)pos)->end_pos;
    size_t len = end_pos - start_pos;
    printf("Handling elements:\n");
    for(size_t i = start_pos; i != end_pos; ++i)
        printf("%d ", data[i]);
    qsort(data + start_pos, len, sizeof(int), cmp);
    printf("\nSorted %zd elements.\n", len);
    pthread_exit(0);
}

void *merge(void *pos) {
    size_t start_pos[2] = {((pos_inf *)pos)[0].start_pos, ((pos_inf *)pos)[1].start_pos};
    size_t end_pos[2] = {((pos_inf *)pos)[0].end_pos, ((pos_inf *)pos)[1].end_pos};

    printf("Handling elements:\n");
    for(size_t i =  start_pos[0]; i != end_pos[1]; ++i)
        printf("%d ", data[i]);
    int temp[end_pos[1] - start_pos[0]];
    size_t dpos = 0, cpos[2] = {start_pos[0], start_pos[1]};
    int dups;
    for(dups = 0; cpos[0] != end_pos[0] && cpos[1] != end_pos[1]; ++dpos) {
        if(data[cpos[0]] > data[cpos[1]]) {
            temp[dpos] = data[cpos[1]];
            ++cpos[1];
        }
        else if(data[cpos[0]] < data[cpos[1]]) {
            temp[dpos] = data[cpos[0]];
            ++cpos[0];
        }
        else {
            temp[dpos] = data[cpos[0]];
            ++cpos[0];
            ++dups;
        }
    }

    for(; cpos[0] != end_pos[0]; ++cpos[0], ++dpos)
        temp[dpos] = data[cpos[0]];
    for(; cpos[1] != end_pos[1]; ++cpos[1], ++dpos)
        temp[dpos] = data[cpos[1]];

    memcpy(data + start_pos[0], temp, (end_pos[1] - start_pos[0]) * sizeof(int));
    printf("\nMerged %zd and %zd elements with %d duplicates.\n",
            end_pos[0] - start_pos[0], end_pos[1] - start_pos[1], dups);
            
    pthread_exit(0);
}

int cmp(const void *a, const void *b) {
    return *(int *)a > *(int *)b;
}
