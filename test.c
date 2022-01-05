#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>

volatile int g_counter = 0;

static void handler(int sig){
	g_counter = -1;//Later gets +1ed so it should be 0
	fprintf(stderr, "Handled SIGINT\n");
}

int main(int argc, char *argv[]){
	sigset_t signal_set;
	int my_pid = 0;
	int max_loop = 0;
	int block_b = 0;
	struct sigaction act = {{0}};
	
	block_b = atoi(argv[4]);
	my_pid = getpid();
	act.sa_handler = handler;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGUSR1);
	sigaction(SIGUSR1, &act, NULL);
	if(argc != 5){
		fprintf(stderr, "Wrong number of arguments. Exiting. . . \n");
		exit(0);
	}
	max_loop = atoi(argv[2]);
	block_b = atoi(argv[4]);
	if(block_b == 0){//Case of not blocking
		for(g_counter = 0; g_counter < max_loop; g_counter++){
			printf("%d: Ding. Counter is %d\n", my_pid, g_counter);
			sleep(1);
		}
	}else if(block_b == 1){//Case of blocking
		for(g_counter = 0; g_counter < max_loop; g_counter++){
			sigprocmask(SIG_BLOCK, &signal_set, NULL);
			
			printf("%d: Ding. Counter is %d\n", my_pid, g_counter);
			if(g_counter >= max_loop / 2 - 1){
				sigprocmask(SIG_UNBLOCK, &signal_set, NULL);
			}
			sleep(1);
		}
	}else{
		fprintf(stderr, "Wrong input on argument -b\n");
	}
	return 0;
}
