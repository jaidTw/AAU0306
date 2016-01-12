/* System Programing 2015 Fall
 * Homework 2
 */
#include <unistd.h>
#include <wait.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "combination.h"
#include "queue.h"

#define READ_END 0
#define WRITE_END 1

typedef struct {
	pid_t pid;
	int id;
	int rpipe[2];
	int wpipe[2];
}host_t;

typedef struct {
	int id;
	int score;
}player_t;

void parse_args(char **argv, int *host_num, int *player_num);
void fork_host(host_t *hosts, int host_id);
void handle_response(player_t *players, char *buf);
void print_ranking(player_t *players, int player_num);
static inline int comp_score(const void* a, const void* b) { return ((player_t *)a)->score < ((player_t *)b)->score; }
static inline int comp_id(const void* a, const void* b) { return ((player_t *)a)->id > ((player_t *)b)->id; }

static char buf[32];

int main(int argc, char **argv) {
	// check args
	if(argc != 3) {
		fprintf(stderr, "Usage : ./bidding_system [host_num : 1~12] [player_num : 4~20]\n");
		exit(-1);
	}

	// parse host & player number.
	int host_num, player_num;
	parse_args(argv, &host_num, &player_num);

	// open pipes and fork hosts
	host_t hosts[host_num];
	for(int idx = 0; idx < host_num; ++idx) {
		hosts[idx].id = idx + 1;
		pipe(hosts[idx].rpipe);
		pipe(hosts[idx].wpipe);
		fork_host(hosts, idx);
	}
	
	// Use I/O multiplex to handle response
	fd_set mread_set, read_set;
	FD_ZERO(&mread_set);

	// Init queue and fd_set for idle host
	int_queue idle_queue;
	int_queue_init(&idle_queue, host_num + 1);
	for(int idx = 0; idx < host_num; ++idx) {
		int_queue_push(&idle_queue, idx);
		FD_SET(hosts[idx].rpipe[READ_END], &mread_set);
	}

	// init players inf.
	player_t players[player_num];
	for(int i = 0; i < player_num; ++i) {
		players[i].id = i + 1;
		players[i].score = 0;
	}

	// generate combinations.
	comb_gen gen;
	comb_gen_init(&gen, player_num, 4);
	int *comb = NULL;
	int done = 0;
	for(;;) {
		if(!done && idle_queue.size != 0) {
			// if all combinations are generated
			if((done = (comb = comb_gen_next(&gen)) == NULL)) continue;

			// Extract idle host from queue.
			int idle_idx;
			int_queue_extract(&idle_queue, &idle_idx);

			// Write to host
			sprintf(buf, "%d %d %d %d\n", comb[0], comb[1], comb[2], comb[3]);
			write(hosts[idle_idx].wpipe[WRITE_END], buf, strlen(buf));
			continue;
		}
		// if all combinations are generated and all hosts are done.
		if(done && idle_queue.size == host_num)	break;
		
		// Check if any host is done, handle the response and push the host back into the idle queue.
		memcpy(&read_set, &mread_set, sizeof(fd_set));
		int ready = select(3 + host_num * 2, &read_set, NULL, NULL, &(struct timeval) {0, 800});
		if(ready <= 0) continue;
		for(int idx = 0; idx < host_num; ++idx) {
			if(!FD_ISSET(hosts[idx].rpipe[READ_END], &read_set)) continue;
			read(hosts[idx].rpipe[READ_END], buf, sizeof(buf));
			handle_response(players, buf);
			int_queue_push(&idle_queue, idx);
		}
	}
	// close all fds, and reap child
	for(int idx = 0; idx < host_num; ++idx) {
		write(hosts[idx].wpipe[WRITE_END], "-1 -1 -1 -1\n", 13);
		close(hosts[idx].wpipe[WRITE_END]);
		close(hosts[idx].rpipe[READ_END]);
		wait(NULL);
	}
	comb_gen_clear(&gen);
	int_queue_clear(&idle_queue);
	
	print_ranking(players, player_num);
	return 0;
}

void parse_args(char **argv, int *host_num, int *player_num) {
	char *hendptr, *pendptr;
	*host_num = strtol(argv[1], &hendptr, 10);
	*player_num = strtol(argv[2], &pendptr, 10);

	// make sure args are numbers.
	if(!((*argv[1] != '\0' && *hendptr == '\0') && (*argv[2] != '\0' && *pendptr == '\0'))) {
		fprintf(stderr, "Invalid arguments.\n");
		fprintf(stderr, "Usage : ./bidding_system [host_num : 1~12] [player_num : 4~20]\n");
		exit(-1);
	}
	// check range
	if(*host_num < 1 || *host_num > 12 || *player_num < 4 || *player_num > 20) {
		fprintf(stderr, "Host number or player number is out of range.\n");
		fprintf(stderr, "Usage : ./bidding_system [host_num : 1~12] [player_num : 4~20]\n");
		exit(-1);
	}
}

void fork_host(host_t* hosts, int idx) {
	sprintf(buf, "%d", hosts[idx].id);
	if((hosts[idx].pid = fork()) < 0) {
		fprintf(stderr, "Error while forking host %d.\n", hosts[idx].id);
		perror(NULL);
		exit(-1);
	}
	else if(hosts[idx].pid == 0) {
		// redirections & close unused pipes.
		dup2(hosts[idx].wpipe[READ_END], STDIN_FILENO);
		dup2(hosts[idx].rpipe[WRITE_END], STDOUT_FILENO);
		for(int other_idx = 0; other_idx < idx; ++other_idx) {
			close(hosts[other_idx].rpipe[READ_END]);
			close(hosts[other_idx].wpipe[WRITE_END]);
		}
		close(hosts[idx].rpipe[READ_END]);
		close(hosts[idx].wpipe[WRITE_END]);
		// execute host.
		execl("./host", "./host", buf, (char *)NULL);
	}
	close(hosts[idx].rpipe[WRITE_END]);
	close(hosts[idx].wpipe[READ_END]);
}

void handle_response(player_t *players, char *buf) {
	int player_id, player_rank;
	char *tok = buf;
	tok = strtok(buf, "\n");
	for(int i = 0; i < 4; ++i) {
		sscanf(tok, "%d %d", &player_id, &player_rank);
		players[player_id - 1].score += (4 - player_rank);
		tok = strtok(NULL, "\n");
	}
}

void print_ranking(player_t* players, int player_num) {
	qsort((void *)players, player_num, sizeof(player_t), comp_score);
	int last_rank, last_score = -1;
	for(int i = 0; i < player_num; ++i) {
		if(players[i].score != last_score) {
			last_score = players[i].score;
			players[i].score = i + 1;
			last_rank = i + 1;
		}
		else {
			last_score = players[i].score;
			players[i].score = last_rank;
		}
	}
	qsort((void *)players, player_num, sizeof(player_t), comp_id);
	for(int i = 0; i < player_num; ++i)
		printf("%d %d\n", players[i].id, players[i].score);
}
