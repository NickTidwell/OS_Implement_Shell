#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include<sys/wait.h>

typedef struct {
	int size;
	char** items;
} tokenlist;

char* get_input(void);
tokenlist* get_tokens(char* input);

tokenlist* new_tokenlist(void);
void add_token(tokenlist* tokens, char* item);
void free_tokens(tokenlist* tokens);
char* checkCommand(char*);
int executeCmd(char*, char*, char**, char**, char**, int, int[], int);
int f_cd(char**);
int f_echo(char**);
int f_exit(char**);

char* builtin_str[] = {
  "cd",
  "echo",
  "exit"
};
int (*builtin_func[]) (char**) = {
	&f_cd,
	&f_echo,
	&f_exit
};
int builin_size = 3;

int f_echo(char** argv) {

	*++argv; //Skip pass the command name
	for (char* c = *argv; c; c = *argv) {
		printf("%s", c);
		if (*++argv) printf(" ");
	}

	printf("\n");
	return 1;
}

int f_cd(char** argv) {

	char* path = *++argv;
	if (*++argv != NULL)
		printf("Too many arguments\n");

	if (path != NULL) {
		int res = chdir(path);

		if (res == -1)
			printf("%s: No such file or directory.", path);
	}
	else chdir(getenv("HOME"));
	free(path);
	return 1;
}

int f_exit(char** argv) {
	return 0;
}


int main()
{
	int bgProcesses[10];
	for (int i = 0; i < 10; i++) {
		bgProcesses[i] = 0;
	}

	while (1) {

		char buff[1000];

		printf("%s@%s : %s > ", getenv("USER"), getenv("MACHINE"), getcwd(buff, 1000)); //This is the prompt

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char* input = get_input();
		char* cmdPath = "";
		char* fOutput = "";
		char* fInput = "";
		int builtIn[3];
		int error = 0;
		int pipe = 0;
		int noWait = 0;
		tokenlist* tokens = get_tokens(input);

		int argNum = 0;
		char** argv1 = malloc(tokens->size * sizeof(char*)); //Create Arguement list size of tokens
		char** argv2 = malloc(tokens->size * sizeof(char*)); //Create Arguement list size of tokens
		char** argv3 = malloc(tokens->size * sizeof(char*)); //Create Arguement list size of tokens
		int argI = 0; //Holds index of next free memory space
		int isCMD = 1;
		for (int i = 0; i < tokens->size; i++) {
			char* token = tokens->items[i];
			//This is the first arguement which is the command
			if (isCMD == 1) {
				isCMD = 0;
				if (strcmp(token, "echo") == 0) {
					cmdPath = "echo";
					builtIn[argNum] = 1;
				}
				else if (strcmp(token, "exit") == 0) {
					cmdPath = "exit";
					builtIn[argNum] = 1;
				}
				else if (strcmp(token, "cd") == 0) {
					cmdPath = "cd";
					builtIn[argNum] = 1;
				}
				else {
					builtIn[argNum] = 0;
					cmdPath = checkCommand(token);
					if (cmdPath == NULL) {
						break;
					}

				}

				switch (argNum)
				{
				case 0:
					//Free Space, assign value, move memory space forward
					argv1[argI] = (char*)malloc(strlen(cmdPath));
					strcpy(argv1[argI], cmdPath);
					argI++;
					break;
				case 1:
					argv2[argI] = (char*)malloc(strlen(cmdPath));
					strcpy(argv2[argI], cmdPath);
					argI++;
					break;
				case 2:
					argv3[argI] = (char*)malloc(strlen(cmdPath));
					strcpy(argv3[argI], cmdPath);
					argI++;
					break;
				default:
					break;
				}



			}
			else if (token[0] == '-') {
				switch (argNum)
				{
				case 0:
					//Free Space, assign value, move memory space forward
					argv1[argI] = (char*)malloc(strlen(token));
					strcpy(argv1[argI], token);
					argI++;
					break;
				case 1:
					argv2[argI] = (char*)malloc(strlen(token));
					strcpy(argv2[argI], token);
					argI++;
					break;
				case 2:
					argv3[argI] = (char*)malloc(strlen(token));
					strcpy(argv3[argI], token);
					argI++;
					break;
				default:
					break;
				}
			}
			//This Check is To see if token is environment variable
			else if (token[0] == '$') {

				memmove(token, token + 1, strlen(token)); //Moves pointer forward trimming off $
				char* cpy = (char*)malloc(strlen(token) + 1);
				strcpy(cpy, token);

				if (getenv(cpy) == NULL) {
					printf("$%s is unknown\n", cpy);
					error = 1;
					break;
				}
				token = (char*)malloc(strlen(getenv(cpy)) + 1); //Allocate New Space for token
				strcpy(token, getenv(cpy)); //Assign token to enveronment variable

				switch (argNum)
				{
				case 0:
					//Free Space, assign value, move memory space forward
					argv1[argI] = (char*)malloc(strlen(token));
					strcpy(argv1[argI], token);
					argI++;
					break;
				case 1:
					argv2[argI] = (char*)malloc(strlen(token));
					strcpy(argv2[argI], token);
					argI++;
					break;
				case 2:
					argv3[argI] = (char*)malloc(strlen(token));
					strcpy(argv3[argI], token);
					argI++;
					break;
				default:
					break;
				}

				free(cpy);
			}
			else if (token[0] == '~') {
				//Replaces ~ with home environment path
				memmove(token, token + 1, strlen(token));
				char* cpy = (char*)malloc(strlen(token));
				strcpy(cpy, token);
				token = (char*)realloc(token, (strlen(token) + strlen(getenv("HOME")) + 2));
				sprintf(token, "%s%s", getenv("HOME"), cpy);

				switch (argNum)
				{
				case 0:
					//Free Space, assign value, move memory space forward
					argv1[argI] = (char*)malloc(strlen(token));
					strcpy(argv1[argI], token);
					argI++;
					break;
				case 1:
					argv2[argI] = (char*)malloc(strlen(token));
					strcpy(argv2[argI], token);
					argI++;
					break;
				case 2:
					argv3[argI] = (char*)malloc(strlen(token));
					strcpy(argv3[argI], token);
					argI++;
					break;
				default:
					break;
				}

				free(cpy);
			}
			else if (token[0] == '>') {

				fOutput = tokens->items[++i];
			}
			else if (token[0] == '<') {

				fInput = tokens->items[++i];
			}
			else if (token[0] == '|') {
				//Run past command save output for next input
				switch (argNum)
				{
				case 0:
					argv1[argI] = NULL;
				case 1:
					argv2[argI] = NULL;
				case 2:
					argv3[argI] = NULL;
				default:
					break;
				}
				argI = 0;
				argNum++;
				isCMD = 1;
			}
			else if (token[0] == '&') {
				//Set variable so later don't wait for command to finish
				noWait = 1;
			}
			else {
				switch (argNum)
				{
				case 0:
					//Free Space, assign value, move memory space forward
					argv1[argI] = (char*)malloc(strlen(token));
					strcpy(argv1[argI], token);
					argI++;
					break;
				case 1:
					argv2[argI] = (char*)malloc(strlen(token));
					strcpy(argv2[argI], token);
					argI++;
					break;
				case 2:
					argv3[argI] = (char*)malloc(strlen(token));
					strcpy(argv3[argI], token);
					argI++;
					break;
				default:
					break;
				}
			}
		}


		switch (argNum)
		{
		case 0:
			argv1[argI] = NULL;
			argv2[0] = NULL;
			argv3[0] = NULL;
		case 1:
			argv2[argI] = NULL;
			argv3[0] = NULL;
		case 2:
			argv3[argI] = NULL;
		default:
			break;
		}
		pid_t pid;
		if (error == 0) {
			int res = executeCmd(fOutput, fInput, argv1, argv2, argv3, argNum, builtIn, noWait);
			pid = res;
			if (res == 0) return 0;
		}
		//add background process to list
		if (noWait == 1) {
			for (int i = 0; i < 10; i++) {
				if (bgProcesses[i] == 0) {
					bgProcesses[i] = pid;
					printf("[%d]\t%d\n", i+1, pid);
					break;
				}
			}
		}
		//check all background processes
		for (int i = 0; i < 10; i++) {
			if (bgProcesses[i] != 0) {
				pid_t bgStatus = waitpid(bgProcesses[i], NULL, WNOHANG);
				if (bgStatus != 0) {
					printf("[%d]+\t%d\n", i+1, bgProcesses[i]);
					bgProcesses[i] = 0;
				}
			}
		}

		free(input);
		free_tokens(tokens);
	}

	return 0;
}

tokenlist* new_tokenlist(void)
{
	tokenlist* tokens = (tokenlist*)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = NULL;
	return tokens;
}

void add_token(tokenlist* tokens, char* item)
{
	int i = tokens->size;

	tokens->items = (char**)realloc(tokens->items, (i + 1) * sizeof(char*));
	tokens->items[i] = (char*)malloc(strlen(item) + 1);
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

char* get_input(void)
{
	char* buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char* newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char*)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char*)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

tokenlist* get_tokens(char* input)
{
	char* buf = (char*)malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist* tokens = new_tokenlist();

	char* tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist* tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
}

char* checkCommand(char* token) {
	int cmdExist = 0;
	const char* delimeter = ":";
	char* pathToken; //Get first path token
	char* rest = (char*)malloc(strlen(getenv("PATH")));
	strcpy(rest, getenv("PATH")); //Needed do not destroy the path
	while ((pathToken = strtok_r(rest, delimeter, &rest)) && cmdExist == 0) {
		char* res = (char*)malloc(strlen(pathToken) + strlen(token) + 2);
		strcpy(res, pathToken);
		strcat(res, "/");
		strcat(res, token);
		//printf("%s\n", res);
		pathToken = strtok(NULL, delimeter); //get next path token

		if (access(res, F_OK) != -1) {
			// file exists
			return res;
			//printf("Exist\n");
		}
		else {
			// file doesn't exist
			//printf("Not Exist\n");
		}
	}

	if (access(token, F_OK) != -1) {
		// file exists
		return token;
		//printf("Exist\n");
	}

	printf("Command not found\n");
	return NULL;


}

int executeCmd(char* output, char* input, char** argv1, char** argv2, char** argv3, int argSize, int builtIn[], int noWait) {

	int status;
	int i;


	// make 2 pipes (argv1 to argv2 and argv2 to argv3); each has 2 fds
	int pipes[4];
	pipe(pipes); //1st pipe
	pipe(pipes + 2); //2nd pipe

	pid_t pid1;
	pid_t pid2;
	pid_t pid3;

	// fork the first child (to execute argv1)
	pid1 = fork();
	if (pid1 == 0)
	{

		if (strcmp(output, "") != 0) {
			//Redirects Childs proccess standard output to file
			int output_fd = open(output, O_WRONLY | O_CREAT | O_TRUNC);
			dup2(output_fd, STDOUT_FILENO);
			dup2(output_fd, STDERR_FILENO);
			close(output_fd);
		}

		if (strcmp(input, "") != 0) {
			int fd0 = open(input, O_RDONLY, 0);
			dup2(fd0, STDIN_FILENO);
			close(fd0);

		}

		// direct argv2 stdout to 1st pipe
		if (argv2[0] != NULL) dup2(pipes[1], 1);

		// close all pipes
		close(pipes[0]);
		close(pipes[1]);
		close(pipes[2]);
		close(pipes[3]);

		if (builtIn[0] == 1) {
			for (int i = 0; i < builin_size; i++) {
				if (strcmp(argv1[0], builtin_str[i]) == 0) {
					int res = (*builtin_func[i])(argv1);
					if (res == 0)
						return 0;
				}
			}
		}
		else {
			execv(argv1[0], argv1);
		}
		exit(0);
	}
	else if (argSize > 0)
	{
		// fork second child (to execute argv2)
		pid2 = fork();
		if (pid2 == 0)
		{
			//direct argv2 read to 1st pipe

			if (argv2[0] != NULL) dup2(pipes[0], 0);

			// replace argv2 stdout to 2nd pipe
			if (argv3[0] != NULL) dup2(pipes[3], 1);

			// close all pipes
			close(pipes[0]);
			close(pipes[1]);
			close(pipes[2]);
			close(pipes[3]);

			if (builtIn[1] == 1) {
				for (int i = 0; i < builin_size; i++) {
					if (strcmp(argv2[0], builtin_str[i]) == 0) {
						int res = (*builtin_func[i])(argv2);
						if (res == 0)
							return 0;
					}
				}
			}
			else {
				execv(argv2[0], argv2);
			}
			exit(0);
		}
		else if (argSize > 1)
		{
			// fork third child
			pid3 = fork();
			if (pid3 == 0)
			{
				// replace arv3 stdin with input of 2nd Pipe
				if (argv3[0] != NULL) dup2(pipes[2], 0);

				// close all pipes
				close(pipes[0]);
				close(pipes[1]);
				close(pipes[2]);
				close(pipes[3]);

				if (builtIn[2] == 1) {
					for (int i = 0; i < builin_size; i++) {
						if (strcmp(argv3[0], builtin_str[i]) == 0) {
							int res = (*builtin_func[i])(argv3);
							if (res == 0)
								return 0;
						}
					}
				}
				else {
					execv(argv3[0], argv3);
				}

				exit(0);
			}
		}
	}


	close(pipes[0]);
	close(pipes[1]);
	close(pipes[2]);
	close(pipes[3]);

	// Wait for all pipe ends

	if (noWait == 0) {
		for (i = 0; i < argSize + 1; i++) {
			wait(&status);
		}
	}

	if (argSize = 0) {
		return pid1;
	}
	else if (argSize = 1) {
		return pid2;
	}
	else if (argSize = 2) {
		return pid3;
	}
	else {
		return 1;
	}
}
