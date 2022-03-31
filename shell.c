// Modify this file for your assignment
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#define BUILTIN_COMMANDS 5
#define BUFFER_SIZE 80

char PWD[1024];
char PATH[1024];
char* HISTORY[1024];

char* builtin[] = {"exit", "help", "cd", "pwd", "history"};

int shell_cd(char** args){
	if(args[1] == NULL){
		fprintf(stderr, "mini-shell: one argument required.\n");
	}
	else if(chdir(args[1]) < 0){
		perror("mini-shell");
	}
	else{
		getcwd(PWD, sizeof(PWD));
	}
	return 1;
}

int shell_exit(char** args){
	return 0;
}

int shell_help(char** args){
	printf("====MINI SHELL====\n");
	printf("Commands: \n");
	printf("\t -help\n");
	printf("\t -exit\n");
	printf("\t -cd dir\n");
	printf("\t -pwd \n");
	printf("\t -history\n");
	return 1;
}

int shell_pwd(char** args){
	printf("%s\n", PWD);
	return 1;
}

int shell_history(char** args){
	int i;
	for(i = 0; i < 1024; i++){
		if(HISTORY[i] != NULL){
			printf("%s\n", HISTORY[i]);
		}
	} 
	return 1;
}

int (* builtin_function[])(char**) = {
	&shell_exit,
	&shell_help,
	&shell_cd,
	&shell_pwd,
	&shell_history
};


void sigint_handler(int sig){
	write(1, "mini-shell terminated\n", 22);
	exit(0);
}

char** split_command_line(char* command){
	int position = 0;
        int no_of_tokens = 64;
        char ** tokens = malloc(sizeof(char *) * no_of_tokens);
        char delim[2] = " ";

	char * token = strtok(command, delim);
        while (token != NULL){
                tokens[position] = token;
                position++;
                token = strtok(NULL, delim);
        }
        tokens[position] = NULL;
        return tokens;
}

char* read_command_line(){
	int position = 0;
        int buf_size = BUFFER_SIZE;
	char * command = (char *)malloc(sizeof(char) * buf_size);
        char c;

	c = getchar();
        while (c != EOF && c != '\n'){
                command[position] = c;
		if(position >= buf_size){
		perror("command exceed size!\n");
		}
		position++;
                c = getchar();
	}
	
	return command;

}

int start_process(char** args){
	int status;
        pid_t pid;
	pid_t wpid;
        pid = fork();

	if(pid == 0){
		if(execvp(args[0], args) == -1){
			perror("mini-shell");
		}

		exit(EXIT_FAILURE);
	}else if(pid < 0){
		perror("mini-shell");
	}else{
		do{
			wpid = waitpid(pid, &status, WUNTRACED);
		}while(!WIFEXITED(status) && !WIFSIGNALED(status));
	}
	return 1;
}

int shell_execute(char** args){
	if(args[0] == NULL){
		return 1;
	}
	int i;
	for(i = 0 ; i < BUILTIN_COMMANDS ; i++){
		if ( strcmp(args[0], builtin[i]) == 0 ){
			int ret_status = (* builtin_function[i])(args);
			return ret_status;
		}
	}

	int ret_status = start_process(args);
	return ret_status;
}

int is_pipe_command(char* command){
	if (strchr(command, '|') == NULL) {
		//printf("is not piped command\n");
		return 0;
	}
	//printf("is piped command\n");
	return 1;
	
}
char** parse_pipe(char* command){
	int position = 0;
        int no_of_tokens = 64;
        char ** tokens = malloc(sizeof(char *) * no_of_tokens);
        char delim[2] = "|";

	char * token = strtok(command, delim);
        while (token != NULL){
                tokens[position] = token;
                position++;
                token = strtok(NULL, delim);
        }
        tokens[position] = NULL;
        return tokens;
}

int exec_pipe(char* command){
	char** parsed_pipecommand = parse_pipe(command);
	char** arguments1 = split_command_line(parsed_pipecommand[0]);
	char** arguments2 = split_command_line(parsed_pipecommand[1]);

	int fd[2];
	if(pipe(fd) == -1){
		perror("Pipe faild.\n");
		exit(1);
	}
	if (fork() == 0){
		close(STDOUT_FILENO);
		dup(fd[1]);
		close(fd[0]);
		close(fd[1]);
		execvp(arguments1[0], arguments1);
		perror("Execvp failed.\n");
		exit(1);
	}
	if (fork() == 0){
		close(STDIN_FILENO);
		dup(fd[0]);
		close(fd[1]);
		close(fd[0]);
		
		execvp(arguments2[0], arguments2);
		perror("Execvp failed.\n");
		exit(1);
	}

	close(fd[0]);
	close(fd[1]);
	wait(0);
	wait(0);

	free(arguments1);
	free(arguments2);
	return 1;
}

void shell_loop(void){
	int status = 1;

        char * command_line;
        char ** arguments;
	char** piped_arguments;
	int history_index = -1;
        while (status){
                printf("mini-shell> ");
                command_line = read_command_line();
		if ( strcmp(command_line, "") == 0 ){
			continue;
		}
		history_index += 1;
		if (history_index >= 1024){
			printf("cmd exceed!");
		}
		else{
			HISTORY[history_index] = command_line;
		}
		
		if (1 == is_pipe_command(command_line)){
			status = exec_pipe(command_line);	
		}
		arguments = split_command_line(command_line);
		status = shell_execute(arguments);
        }
	free(command_line);
	free(arguments);
}

int main(int argc, char** argv){
       	alarm(180);
	signal(SIGINT, sigint_handler);
	getcwd(PWD, sizeof(PWD));
	strcpy(PATH, PWD);
	strcat(PATH, "/cmds/");

	shell_loop();
       	return 0;
}
