#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <queue>
#include <algorithm>
#include <pthread.h>

#define ERR_EXIT(str, ...) { fprintf(stderr, str, ## __VA_ARGS__); exit(-1); }

struct pos_inf{
    size_t start_pos;
    size_t end_pos;
};

struct job_t{
    void *(*rtn)(void *);
    void *arg;
};

int parsearg(char *);
void *sort(void *);
void *merge(void *);

static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t job_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t done_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t wait_cond = PTHREAD_COND_INITIALIZER;
static std::queue<job_t> job_queue;
static size_t done_num;
static int *data;
static int seg_size;

void *thread_rtn(void *) {
    for(;;) {
        pthread_mutex_lock(&job_mutex);
        while(job_queue.empty()) {
            pthread_cond_wait(&wait_cond, &job_mutex);
        }
        job_t job = job_queue.front();
        job_queue.pop();
        pthread_mutex_unlock(&job_mutex);

        job.rtn(job.arg);

        pthread_mutex_lock(&done_mutex);
        ++done_num;
        pthread_mutex_unlock(&done_mutex);
    }
    pthread_exit(0);
}

void add_job(void *(*rtn)(void *), void *arg) {
    pthread_mutex_lock(&job_mutex);
    job_queue.push((job_t){rtn, arg});
    pthread_cond_signal(&wait_cond);
    pthread_mutex_unlock(&job_mutex);
};

int main(int argc, char **argv) {
    if(argc != 2) 
        ERR_EXIT("Usage : ./merger [segment_size]\n");
    seg_size = parsearg(argv[1]);
    
    size_t num;
    scanf("%zd", &num);
    data = (int *)malloc(num * sizeof(int));
    if(data == NULL)
        ERR_EXIT("Memory allocation failed, please try again later.");

    for(size_t i = 0; i < num; ++i)
        scanf("%d", &data[i]);
    
    size_t seg_num = ceil((double)num / seg_size);
    pos_inf pos[seg_num];
    for(size_t i = 0; i < seg_num; ++i) {
        pos[i].start_pos = i * seg_size;
        pos[i].end_pos = (i + 1) * seg_size;
    }
    pos[seg_num - 1].end_pos = num;

    // sort
    pthread_t *tid = new pthread_t[seg_num];
    for(size_t i = 0; i < seg_num; ++i)
        pthread_create(&tid[i], NULL, thread_rtn, NULL);

    for(size_t i = 0; i < seg_num; ++i)
        add_job(sort, (void *)&pos[i]);
    
    while(done_num != seg_num);
    done_num = 0;

    // merge
    while(ceil(seg_num / 2)) {
        size_t thread_num = floor((double)seg_num / 2);
        seg_num = ceil((double)seg_num / 2);
        seg_size *= 2;

        // compute segment interval
        pos_inf pos[thread_num][2];
        for(size_t i = 0; i < thread_num; ++i) {
            pos[i][0].start_pos = i * seg_size;
            pos[i][1].start_pos = pos[i][0].end_pos = i * seg_size + seg_size / 2;
            pos[i][1].end_pos = (i + 1) * seg_size;
        }
        if(seg_size * thread_num > num)
            pos[thread_num - 1][1].end_pos = num;

        for(size_t i = 0; i < thread_num; ++i)
            add_job(merge, (void *)&pos[i]);
        
        while(done_num != thread_num);
        done_num = 0;
    }

    for(size_t i = 0; i < num - 1; ++i)
        printf("%d ", data[i]);
    printf("%d\n\n", data[num - 1]);
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
    pthread_mutex_lock(&output_mutex);
    printf("Handling elements:\n");
    for(size_t i = start_pos; i != end_pos - 1; ++i)
        printf("%d ", data[i]);
    printf("%d\nSorted %zd elements.\n", data[end_pos - 1], len);
    pthread_mutex_unlock(&output_mutex);
    std::sort(data + start_pos, data + end_pos);

    return 0;
}

void *merge(void *pos) {
    size_t start_pos[2] = {((pos_inf *)pos)[0].start_pos, ((pos_inf *)pos)[1].start_pos};
    size_t end_pos[2] = {((pos_inf *)pos)[0].end_pos, ((pos_inf *)pos)[1].end_pos};

    int *temp = (int *)malloc(sizeof(int) * end_pos[1] - start_pos[0]);
    if(!temp)
        ERR_EXIT("Error while allocating space for thread.\n");
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

    pthread_mutex_lock(&output_mutex);
    printf("Handling elements:\n");
    for(size_t i =  start_pos[0]; i != end_pos[1] - 1; ++i)
        printf("%d ", data[i]);
    printf("%d\nMerged %zd and %zd elements with %d duplicates.\n", data[end_pos[1] - 1],
            end_pos[0] - start_pos[0], end_pos[1] - start_pos[1], dups);
    pthread_mutex_unlock(&output_mutex);
    memcpy(data + start_pos[0], temp, (end_pos[1] - start_pos[0]) * sizeof(int));
    free(temp);
    return 0;
}
