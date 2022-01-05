#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>

#define ALARM_TIME 20
#define COMMAND_SIZE 512
#define NUMBER_OF_ARGS 10

struct programme{
	int pid;
	char *name;
	char *arguments[NUMBER_OF_ARGS]; 
	struct programme *next;
	int b_running;
};

typedef struct programme Prog_T;

struct programme *volatile header;

void delete_node_handler(Prog_T *prev, int is_header);
void delete_node(Prog_T *prev, int is_header);
Prog_T *prev_node(int target_pid, int *is_header);
void addprogramme(int *is_first);
void send_sig(int pid);
void error_check(int return_val, int line);

static void handler(int sig) {
	int return_pid = 0;
	int status = 0;
	int is_header = 0;
	Prog_T *prev = NULL;
	Prog_T *runner = NULL;
	Prog_T *running = NULL;
	int return_val = 0;
	
	if(sig == SIGCHLD){//Handler for the SIGCHLD
		return_pid = waitpid(-1, &status, WNOHANG );
		if(return_pid == 0){//If the child didn't exit
			return;
		}else if(WIFSIGNALED(status) || WIFEXITED(status)){//If the child exited or was signaled too
			fprintf(stderr, "Got it\n");
			prev = prev_node(return_pid, &is_header);
			if(is_header == 1 && prev->next != NULL){//In case its the first node
				prev->next->b_running = 1;
				return_val = kill(prev->next->pid, SIGCONT);
				error_check(return_val, __LINE__);
			}else if(is_header == 1 && prev->next == NULL){//In case its the first and last node
				header->pid = 0;
				header->b_running = 0;
				return;
			}else if(is_header == 0 && prev->next->next == NULL){//In case its the last node
				header->b_running = 1;
				return_val = kill(header->pid, SIGCONT);
				error_check(return_val, __LINE__);
			}else if(is_header == 0 && prev->next->next != NULL){//In case its a node somewhere in the middle
				prev->next->next->b_running = 1;
			}
			delete_node_handler(prev, is_header);//Calling the function to delete node
		}
		running = header;
		if(header->pid == 0){//In case list is empty exit
			return;
		}
		while(1){//Swap out the Boolean ID's on if a programme is running(Fixing the boolean values)
			if(running->next == NULL){
				return;
			}
			if(running->next->b_running == 1){
				running->b_running = 1;
				running->next->b_running = 0;
				break;
			}
			running = running->next;
		}
		return_val = kill(getpid(), SIGALRM);
		error_check(return_val, __LINE__);
	}
	else if(sig == SIGALRM){//Handler for the SIGALRM
		runner = header;
		if(header->pid == 0){//Exit in case the list is empty
			return;
		}
		while(1){//Swapping out the Boolean IDs on if a programme is running also stopping and conting
			if(runner->b_running == 1 && runner->next == NULL){//If its the last (could be only) node running
				runner->b_running = 0;
				header->b_running = 1;
				return_val = kill(runner->pid, SIGSTOP);
				error_check(return_val, __LINE__);
				return_val = kill(header->pid, SIGCONT);
				error_check(return_val, __LINE__);
				return;
			}
			if(runner->b_running == 1 && runner->next != NULL){
				runner->b_running = 0;
				runner->next->b_running = 1;
				return_val = kill(runner->pid, SIGSTOP);
				error_check(return_val, __LINE__);
				return_val = kill(runner->next->pid, SIGCONT);
				error_check(return_val, __LINE__);
				return;
			}
			runner = runner->next;
		}	
	}
	
}
void error_check(int return_val, int line){
	if(return_val == -1){
		fprintf(stderr, "System call error at line: %d. Exiting. . . \n", line);
		exit(0);
	}
}

void addprogramme(int *is_first){//Adds a programme into the list 
	Prog_T *node_to_add = NULL;
	Prog_T *runner = NULL;
	int i = 0;
	char get_buffer[COMMAND_SIZE] = {'\0'};
	char *strtok_p = NULL;
	int return_val = 0;

//Mallocing and setting up the node to be added:BEGIN	
	if(*is_first == 0){
		header = malloc(sizeof(struct programme));
		if(header == NULL){
			fprintf(stderr, "Malloc failed at line %d. Exiting. . . \n", __LINE__);
			exit(0);
		}
		header->next = NULL;
		node_to_add = header;//If indeed first then the header is the node to be added
	}else{
		node_to_add = (Prog_T *)malloc(sizeof(struct programme));//mallocing the node to be added
		if(header->pid == 0){
			node_to_add = header;
			header->b_running = 1;
		}
	}
//Mallocing and setting up the node to be added:END
	runner = header;
	while(runner->next != NULL){
		runner = runner->next;
	}
	//Sequence of getting the arguments: BEGIN
	fgets(get_buffer, COMMAND_SIZE, stdin);
	i = 0;
	strtok_p = strtok(get_buffer," "); 
	node_to_add->arguments[i] = (char *)malloc(sizeof(char) * strlen(strtok_p));
	if(node_to_add->arguments[i] == NULL){
		fprintf(stderr, "Malloc failed at line %d\n", __LINE__);	
		exit(0);
	}
	strcpy(node_to_add->arguments[i], strtok_p);
	if(node_to_add->arguments[i][strlen(node_to_add->arguments[i]) - 1] == '\n'){
		node_to_add->arguments[i][strlen(node_to_add->arguments[i]) - 1] = '\0';
	}
	node_to_add->name = (char *)malloc(sizeof(char) * strlen(strtok_p));
	if(node_to_add->name == NULL){
		fprintf(stderr, "Malloc failed at line %d\n", __LINE__);	
		exit(0);
	}
	strcpy(node_to_add->name, node_to_add->arguments[i]);
	fprintf(stderr, "%s\n", node_to_add->arguments[i]);
	i++;
	while(i){
		strtok_p = strtok(NULL, " ");
		if(strtok_p == NULL){
			i++;
			break;
		}
		node_to_add->arguments[i] = (char *)malloc(sizeof(char) * strlen(strtok_p));
		strcpy(node_to_add->arguments[i], strtok_p);
		i++;
	}
	node_to_add->arguments[i] = NULL;
	//Sequence of getting the arguments: END
	if(*is_first == 1){
		runner->next = node_to_add;
	}
	node_to_add->next = NULL;
	node_to_add->pid = fork();
	if(node_to_add->pid == 0){
		execv(node_to_add->name, node_to_add->arguments);
		exit(0);
	}else{
		if(*is_first == 0){
			node_to_add->b_running = 1;
			*is_first = 1;
		}else{
			node_to_add->b_running = 0;
			return_val = kill(node_to_add->pid, SIGSTOP);
			error_check(return_val, __LINE__);
		}
	}
}

Prog_T *prev_node(int target_pid, int *is_header){
	Prog_T *runner = NULL;

	runner = header;
	if(runner->pid == target_pid){
		*is_header = 1;
		return(runner);
	}
	while(runner->next != NULL){
		if(runner->next->pid == target_pid){
			*is_header = 0;	
			return(runner);
		}
		runner = runner->next;
	}
	return NULL;
}

void printinfo(){
	Prog_T *runner = NULL;
	
	runner = header;
	if(runner == header && runner->pid == 0 && runner->b_running == 0){
		printf("List is empty.\n");
		return;
	}
	while(runner->next != NULL){
		printf("%d, pid: %d, name: %s\n",runner->b_running, runner->pid, runner->name);
		runner = runner->next;
	}	
	printf("%d, pid: %d, name: %s\n", runner->b_running, runner->pid, runner->name);
}

void delete_node(Prog_T *prev, int is_header){//Deletes a node from the list
	Prog_T *temp;
	int return_val = 0;

	if(is_header == 1){
		temp = header;
		return_val = kill(temp->pid, SIGTERM);
		error_check(return_val, __LINE__);
		header = header->next;
		free(temp);
		temp = NULL;
	}
	if(is_header == 0){
		temp = prev->next;
		return_val = kill(temp->pid, SIGTERM);
		error_check(return_val, __LINE__);
		prev->next = prev->next->next;
		free(temp);
		temp = NULL;
	} 
}

void delete_node_handler(Prog_T *prev, int is_header){//Deleting the node from the handler
	Prog_T *temp;

	if(is_header == 1){
		temp = header;
		header = header->next;
		free(temp);
	}
	if(is_header == 0){
		temp = prev->next;
		prev->next = prev->next->next;
		free(temp);
	} 
}

void send_sig(int pid){
	int return_val = 0;
	
	return_val = kill(pid, SIGUSR1);
	error_check(return_val, __LINE__);
}

int clear_list(){//Clearing the list before exiting
	Prog_T *temp = NULL;
	Prog_T *runner = NULL;
	int i = 0;
	int return_val = 0;
	int programmes = 0;

	runner = header;
	if(header->pid == 0 && header->b_running == 0){
		free(header);
		return(-1);
	}
	do{
		temp = runner;
		runner = runner->next;
		free(temp->name);
		for(i = 0; i < 10; i++){
			if(temp->arguments[i] == NULL){
				break;
			}
			free(temp->arguments[i]);
		}
		temp->next = NULL;
		return_val = kill(temp->pid, SIGKILL);
		error_check(return_val, __LINE__);
		free(temp);
		programmes++;
	}while(runner != NULL);
	return(programmes);
}
	
void wait_processes(int limit){//Waiting for the processes
	int i = 0;
	
	if(limit == -1){
		return;
	}
	for(; i < limit; i++){
		waitpid(-1, NULL, 0);
	}
}
int main(int argc, char *argv[]){
	sigset_t signal_set;
	char option[10] = {'\0'};//Option variable used to pick option(string)
	int is_first = 0;//Is first binary value 0 for true 1 for false
	int target_pid = 0;//term pid variable used by functions
	struct sigaction action = {{0}};//timer variables
	struct itimerval timer = {{0}};
	int return_val = 0;
	int programmes = 0;

	//These are used for the handler of the signals
	action.sa_handler = handler;
	action.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &action, NULL);
	sigaction(SIGCHLD, &action, NULL);
	//These are used for the handler of the signals
	timer.it_interval.tv_sec = ALARM_TIME;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = ALARM_TIME;
	timer.it_value.tv_usec = 0;
	sigemptyset(&signal_set);
	return_val = sigaddset(&signal_set, SIGALRM);
	error_check(return_val, __LINE__);
	return_val = sigaddset(&signal_set, SIGCHLD);
	error_check(return_val, __LINE__);
	signal(SIGUSR1, SIG_IGN);

	while(1){
		scanf("%s", option);
		if(strcmp(option, "exec") == 0){//Execing programme option
			if(is_first == 0){
				return_val = setitimer(ITIMER_REAL, &timer, NULL);
				error_check(return_val, __LINE__);
			}
			addprogramme(&is_first);
		}
		if(strcmp(option, "term") == 0){
			scanf("%d", &target_pid);
			error_check(return_val, __LINE__);
			return_val = kill(target_pid, SIGTERM);
			error_check(return_val, __LINE__);
			error_check(return_val, __LINE__);
		}
		if(strcmp(option, "info") == 0){
			printinfo();
		}
		if(strcmp(option, "sig") == 0){
			scanf("%d", &target_pid);
			send_sig(target_pid);
		}
		if(strcmp(option, "quit") == 0){
			return_val = sigprocmask(SIG_BLOCK, &signal_set, NULL);
			error_check(return_val, __LINE__);
			programmes = clear_list();
			wait_processes(programmes);
			exit(0);
		}
	}
	return 0;
}
