/* System Programing 2015 Fall
 * Homework 2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

void parse_args(char **argv, char *index, int *host_id, int *key);
char buf[32];

int main(int argc, char **argv) {
	if(argc != 4) {
		fprintf(stderr, "Usage : ./player [host_id : 1~12] [player_index : A~D] [random_key]\n");
		exit(-1);
	};
    srand(time(NULL) ^ (getpid() << 16));
	char index;
	int host_id, key;
	parse_args(argv, &index, &host_id, &key);

	sprintf(buf, "./host%d_%c.FIFO", host_id, index);
	int rfifo = open(buf, O_RDONLY);
	sprintf(buf, "./host%d.FIFO", host_id);
	int wfifo = open(buf, O_WRONLY);

	int money[4];
	for(int rnd = 0; rnd < 10; ++rnd) {
		read(rfifo, buf, 32);
		sscanf(buf, "%d %d %d %d", &money[0], &money[1], &money[2], &money[3]);
		sprintf(buf, "%c %d %d\n", index, key, (money[index - 65] - rand() % 100) * (rnd % 2));
		write(wfifo, buf, strlen(buf));
	}

	close(wfifo);
	close(rfifo);
	return 0;
}

void parse_args(char **argv, char *index, int *host_id, int *key) {
	*index = argv[2][0];
	char *hendptr, *kendptr;
	*host_id = strtol(argv[1], &hendptr, 10);
	*key = strtol(argv[3], &kendptr, 10);

	if(!(*argv[1] != '\0' && *hendptr == '\0') || !(*argv[3] != '\0' && *kendptr == '\0')) {
		fprintf(stderr, "Invalid argument.\n");
		fprintf(stderr, "Usage : ./player [host_id : 1~12] [player_index : A~D] [random_key]\n");
		exit(-1);
	}
	if(strlen(argv[2]) > 1 || *index < 65 || *index > 68) {
		fprintf(stderr, "Invalid argument.\n");
		fprintf(stderr, "Usage : ./player [host_id : 1~12] [player_index : A~D] [random_key]\n");
		exit(-1);
	}
	if(*host_id < 1 || *host_id > 12) {
		fprintf(stderr, "Host number is out of range.\n");
		fprintf(stderr, "Usage : ./player [host_id : 1~12] [player_index : A~D] [random_key]\n");
		exit(-1);
	}
	if(*key < 0 || *key > 65535) {
		fprintf(stderr, "Invalid key.\n");
		fprintf(stderr, "Usage : ./player [host_id : 1~12] [player_index : A~D] [random_key]\n");
		exit(-1);
	}
}
