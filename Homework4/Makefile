all:
	clang -std=gnu11 -O3 -c combination.c
	clang -std=gnu11 -O3 -c queue.c
	clang -std=gnu11 -O3 combination.c queue.c bidding_system.c -o bidding_system
	clang -std=gnu11 -O3 host.c -o host
	clang -std=gnu11 -O3 player.c -o player
	clang -std=gnu11 -O3 player_bonus.c -o player_bonus
	rm combination.o
	rm queue.o
clean:
	rm bidding_system
	rm host
	rm player
	rm player_bonus

