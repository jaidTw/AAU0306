/* System Programing 2015 Fall
 * Homework 2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/select.h>

typedef struct{
	pid_t pid;
	int id;
	int score;
	int money;
	int pay;
	int key;
	int fifo;
	char alpha;
}player_t;

typedef struct{
	int idx;
	int pay;
}cmp_t;

typedef struct{
	int idx;
	int rank;
}rank_t;

void parse_args(char **argv, int *host_id);
void create_fifos(int host_id);
void open_fifos(player_t *players, int host_id);
void unlink_fifos(int host_id);
void fork_player(player_t* players, int host_id, int idx);
void read_response(int rfifo);
void parse_response(player_t *players);
int decide_winner(player_t *players);
void set_alpha(player_t *players, int win_idx);
void write_ranking(player_t *players);
int strncntchr(char *str, char chr, int len);
static inline int cmp_pay(const void *a, const void *b) { return ((cmp_t *)a)->pay < ((cmp_t *)b)->pay; }
static inline int cmp_rank(const void *a, const void *b) { return ((rank_t *)a)->rank < ((rank_t *)b)->rank; }
static inline int cmp_idx(const void *a, const void *b) { return ((rank_t *)a)->idx > ((rank_t *)b)-> idx; }

static const int A = 0;
static const int B = 1;
static const int C = 2;
static const int D = 3;

static char buf[64];

int main(int argc, char **argv) {
	srand(time(NULL));
	if(argc != 2) {
		fprintf(stderr, "Usage : ./host [host_id : 1~12]\n");
		exit(-1);
	}

	int host_id;
	parse_args(argv, &host_id);
	create_fifos(host_id);

	// read input and fork players
	player_t players[4];
	for(;;) {
		scanf("%d %d %d %d", &players[A].id, &players[B].id, &players[C].id, &players[D].id);
		if(players[A].id == -1 && players[B].id == -1
			&& players[C].id == -1 && players[D].id == -1) break;

		// init players and fork
		for(int idx = A; idx <= D; ++idx) {
			players[idx].score = 0;
			players[idx].pay = 0;
			players[idx].money = 0;
			players[idx].alpha = 0;
			players[idx].key = rand() % 65536;
			fork_player(players, host_id, idx);
		}
			
		// open fifos.
		int rfifo;
		open_fifos(players, host_id);
		sprintf(buf, "./host%d.FIFO", host_id);
		rfifo = open(buf, O_RDONLY | O_NONBLOCK);
		// starts 10 rounds.
		for(int i = 0; i < 10; ++i) {
			// calculate moeny
			for(int idx = A; idx <= D; ++idx)
				players[idx].money += 1000 - players[idx].pay * players[idx].alpha;

			// write to fifos
			sprintf(buf, "%d %d %d %d\n", players[A].money, players[B].money, players[C].money, players[D].money);
			for(int idx = A; idx <= D; ++idx)
				write(players[idx].fifo, buf, strlen(buf));			

			read_response(rfifo);
			parse_response(players);
			int win_idx = decide_winner(players);
			for(int idx = A; idx <= D; ++idx)
				players[idx].alpha = (idx == win_idx);
			players[win_idx].score += (win_idx != -1);
		}
		// close fifos
		close(rfifo);
		for(int idx = A; idx <= D; ++idx) {
			close(players[idx].fifo);
			wait(NULL);
		}
		// write result to bidding system
		write_ranking(players);
	}
	unlink_fifos(host_id);
	return 0;
}

void parse_args(char **argv, int *host_id) {
	char *endptr;
	*host_id = strtol(argv[1], &endptr, 10);

	// make sure args are numbers.
	if(!(*argv[1] != '\0' && *endptr == '\0')) {
		fprintf(stderr, "Invalid argument.\n");
		fprintf(stderr, "Usage : ./host [host_id : 1~12]\n");
		exit(-1);
	}
	// check range
	if(*host_id < 1 || *host_id > 12) {
		fprintf(stderr, "Host number is out of range.\n");
		fprintf(stderr, "Usage : ./host [host_id : 1~12]\n");
		exit(-1);
	}
}

void create_fifos(int host_id) {
	sprintf(buf, "./host%d.FIFO", host_id);
	mkfifo(buf, 0777);
	sprintf(buf, "./host%d_A.FIFO", host_id);
	mkfifo(buf, 0777);
	sprintf(buf, "./host%d_B.FIFO", host_id);
	mkfifo(buf, 0777);
	sprintf(buf, "./host%d_C.FIFO", host_id);
	mkfifo(buf, 0777);
	sprintf(buf, "./host%d_D.FIFO", host_id);
	mkfifo(buf, 0777);
}

void open_fifos(player_t *players, int host_id) {
	sprintf(buf, "./host%d_A.FIFO", host_id);
	players[A].fifo = open(buf, O_WRONLY);
	sprintf(buf, "./host%d_B.FIFO", host_id);
	players[B].fifo = open(buf, O_WRONLY);
	sprintf(buf, "./host%d_C.FIFO", host_id);
	players[C].fifo = open(buf, O_WRONLY);
	sprintf(buf, "./host%d_D.FIFO", host_id);
	players[D].fifo = open(buf, O_WRONLY);
}

void unlink_fifos(int host_id) {
	sprintf(buf, "./host%d.FIFO", host_id);
	unlink(buf);
	sprintf(buf, "./host%d_A.FIFO", host_id);
	unlink(buf);
	sprintf(buf, "./host%d_B.FIFO", host_id);
	unlink(buf);
	sprintf(buf, "./host%d_C.FIFO", host_id);
	unlink(buf);
	sprintf(buf, "./host%d_D.FIFO", host_id);
	unlink(buf);
}

void fork_player(player_t *players, int host_id, int idx) {
	char argv[4][32];
	sprintf(argv[0], "./player");
	sprintf(argv[1], "%d", host_id);
	sprintf(argv[2], "%c", idx + 65);
	sprintf(argv[3], "%d", players[idx].key);
	if((players[idx].pid = fork()) < 0) {
		fprintf(stderr, "Error while forking player %c.\n", idx + 65);
		perror(NULL);
		exit(-1);
	}
	else if(players[idx].pid == 0) {
		execl("./player", argv[0], argv[1], argv[2], argv[3], (char *)NULL);
	}
}

void read_response(int rfifo) {
	char *s = buf;
	int len, line_count = 0;
	while(line_count < 4) {
		if((len = read(rfifo, s, 1)) <= 0)
			select(0, NULL, NULL, NULL, &(struct timeval) {0, 1});
		else {
			line_count += strncntchr(s, '\n', len);
			s += len;
		}
	}
	*s = '\0';
}

void parse_response(player_t *players) {
	char *tok = strtok(buf, "\n");
	char idx;
	int key, pay;
	for(int i = 0; i < 4; ++i) {
		sscanf(tok, "%c %d %d", &idx, &key, &pay);
		if(players[idx - 65].key != key) {
			fprintf(stderr, "Recieve response from player %c with error key! aborting...\n", idx);
			exit(-1);
		}
		else if(pay > players[idx - 65].money) {
			fprintf(stderr, "Recieve response from player %c with announcing the pay higher than his/her own money! aborintg...\n", idx);
			exit(-1);
		}
		else
			players[idx - 65].pay = pay;
		if(i < 3)
			tok = strtok(NULL, "\n");
	}
}

int decide_winner(player_t *players) {
	cmp_t pays[5];
	for(int idx = A; idx <= D; ++idx) {
		pays[idx].pay = players[idx].pay;
		pays[idx].idx = idx;
	}
	pays[4].pay = -1;
	pays[4].idx = -1;
	qsort(pays, 4, sizeof(cmp_t), cmp_pay);

	if(pays[0].pay != pays[1].pay) return pays[0].idx;
	int i = 1;
	while(pays[0].pay == pays[i].pay) i++;
	return pays[i].idx;
}

void write_ranking(player_t *players) {
	rank_t rank[4];
	for(int idx = A; idx <= D; ++idx) {
		rank[idx].rank = players[idx].score;
		rank[idx].idx = idx;
	}
	qsort(rank, 4, sizeof(rank_t), cmp_rank);
	int last_rank, last_score = -1;
	for(int i = 0; i < 4; ++i) {
		if(rank[i].rank != last_score) {
			last_score = rank[i].rank;
			rank[i].rank = i + 1;
			last_rank = i + 1;
		}
		else {
			last_score = rank[i].rank;
			rank[i].rank = last_rank;
		}
	}
	qsort(rank, 4, sizeof(rank_t), cmp_idx);
	for(int i = 0; i < 4; ++i) 
		printf("%d %d\n", players[rank[i].idx].id, rank[i].rank);
	
	fflush(stdout);
}

int strncntchr(char *str, char chr, int len) {
	int cnt = 0;
	for(char *end = str + len; str != end; ++str)
		cnt += (*str == chr);
	return cnt;
}	
