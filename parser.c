#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef struct {
	int size;
	char **items;
} tokenlist;

tokenlist *new_tokenlist(void);
tokenlist *get_tokens(char *input, char *delims);
char *get_input(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

char* cmdSearch(char *cmd);
void cmdExecute(tokenlist *tokens);

void echo(tokenlist *tokens);
void cd(tokenlist *tokens);
void jobs(void);
void exitPrgm(void);

static void (*fp) (tokenlist*);

/* variables to store name of files for i/o redirection */
static char* F_OUT;
static char* F_IN;

static int CMDS_EXECUTED = 0;
static int PIPE_COUNT;
static int NO_WAIT;

int bgProcesses[10];
char * bgCommands[10];

int main() {

	for (int i = 0; i < 10; i++) {
		bgProcesses[i] = 0;
		bgCommands[i] = NULL;
	}

	while (1) {
		F_OUT = NULL;
		F_IN = NULL;
		PIPE_COUNT = 0;
		NO_WAIT = 0;



		printf("%s@%s : %s > ", getenv("USER"), getenv("MACHINE"), getenv("PWD"));

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		tokenlist *tokens = get_tokens(input, " ");

		/* checks to see if cmd is a built-in cmd */
		char *cmd = tokens->items[0];
		char* cmdPath;
		if (strcmp(cmd, "exit") == 0)
            exitPrgm();
		else if (strcmp(cmd, "jobs") == 0) {
			jobs();
			continue;
		} else if (strcmp(cmd, "cd") == 0)
			fp = cd;
		else if (strcmp(cmd, "echo") == 0)
			fp = echo;
		else {
			fp = cmdExecute;
			/* checks if cmd is a valid cmd */
			cmdPath = cmdSearch(cmd);
			if (cmdPath == NULL) continue;
			tokens->items[0] = cmdPath;
		}
		/* command parsing - expanding varaibles */
		for (int i = 1; i < tokens->size; i++) {
			char* token = tokens->items[i];

			/* This Check is To see if token is environment variable */
			if (token[0] == '$') {
				memmove(token, token + 1, strlen(token)); /* Moves pointer forward trimming off $ */
				char* envVar = getenv(token);  /* Used to check if token is an existing environment variable */
				if (envVar != NULL) {
					tokens->items[i] = (char*)realloc(tokens->items[i], strlen(envVar) + 1); /* Allocate New Space for token */
				} else {
                    tokens->items[i] = (char*)realloc(tokens->items[i], 1);
					envVar = "\0";  /* token was not an existing environment variable */
				}
				strcpy(tokens->items[i], envVar); /* Assign token to enveronment variable */
			} else if (token[0] == '~') {
				/* Replaces ~ with home environment path */
				memmove(token, token + 1, strlen(token));
				char* cpy = calloc(strlen(token) + 1, sizeof(char));
				strcpy(cpy, token);
				token = (char*)realloc(token, (strlen(token) + strlen(getenv("HOME")) + 2));
				sprintf(token, "%s%s", getenv("HOME"), cpy);
			} else if (token[0] == '<') {
				tokens->items[i] = 0;
				if (++i < tokens->size) {
					F_IN = calloc(strlen(tokens->items[i]) + 1, sizeof(char));
					strcpy(F_IN, tokens->items[i]);
				}
			} else if (token[0] == '>') {
				tokens->items[i] = 0;
				if (++i < tokens->size) {
					F_OUT = calloc(strlen(tokens->items[i]) + 1, sizeof(char));
					strcpy(F_OUT, tokens->items[i]);
				}
			} else if (token[0] == '|') {
				++PIPE_COUNT;
				if (++i < tokens->size) {
					cmdPath = cmdSearch(tokens->items[i]);		/* checks validity of piped cmd */
					if (cmdPath == NULL) break;
					tokens->items[i] = realloc(tokens->items[i], strlen(cmdPath) + 1);
					strcpy(tokens->items[i], cmdPath);
				}
			} else if (token[0] == '&') {
				++NO_WAIT;
				for (int i = 0; i < 10; i++) {
					if (bgProcesses[i] == 0) {
						bgCommands[i] = calloc(strlen(tokens->items[0]) + 1, sizeof(char));
						strcpy(bgCommands[i], tokens->items[0]);
						for (int j = 1; j < tokens->size; j++) {
							bgCommands[i] = (char*)realloc(bgCommands[i], (strlen(bgCommands[i]) + strlen(tokens->items[j]) + 2));
							strcat(bgCommands[i], " ");
							strcat(bgCommands[i], tokens->items[j]);
						}
						break;
					}
				}
			}
		}
		if (cmdPath == NULL && PIPE_COUNT > 0) continue;

		/* executing command */
		fp(tokens);

        free(input);
		free_tokens(tokens);

		for (int i = 0; i < 10; i++) {
			if (bgProcesses[i] != 0) {
				int status = waitpid(bgProcesses[i], NULL, WNOHANG);
				if (status != 0) {
					printf("[%d]+\t%d\t%s\n", i+1, bgProcesses[i], bgCommands[i]);
					bgProcesses[i] = 0;
				}
			}
		}
	}
	return 0;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = NULL;
	return tokens;
}

tokenlist *get_tokens(char *input, char* delims)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, delims);
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, delims);
	}

	tokens->items = (char**)realloc(tokens->items, (tokens->size + 1)*sizeof(char*));
	tokens->items[tokens->size] = NULL;

	free(buf);
	return tokens;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

void add_token(tokenlist *tokens, char *item)
{
    if (tokens != NULL) {
        int i = tokens->size;
	    tokens->items = (char **) realloc(tokens->items, (i + 1) * sizeof(char *));
	    tokens->items[i] = (char *) malloc(strlen(item) + 1);
        strcpy(tokens->items[i], item);
	    tokens->size += 1;
    }
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
}

char* cmdSearch (char *cmd) {
    char* path = (char*)malloc(strlen(getenv("PATH")));
    strcpy(path, getenv("PATH"));
	tokenlist *pathTokens = get_tokens(path, ":");	/* retrieving PATH and separating paths into tokens */
	char *cmdToAppend = (char*)malloc(strlen(cmd) + 1);
	strcpy(cmdToAppend, "/");
	strcat(cmdToAppend, cmd);	/* adding '/' before cmd */
	for (int i = 0; i < pathTokens->size; i++) {
		char *token = pathTokens->items[i];
		token = (char*)realloc(token, strlen(cmdToAppend) + strlen(token) + 1);
		strcat(token, cmdToAppend);		/* concat one of the paths to '/[cmd]' */
		if (access(token, F_OK) == 0) return token;		/* checks if cmd exists in file path */
	}
	printf("%s: command not found\n", cmd);
    free_tokens(pathTokens);
    free(cmdToAppend);
    free(path);
	return NULL;
}

void cmdExecute(tokenlist *tokens) {
	if (PIPE_COUNT > 0) {
		/* puts commands separated by pipes into separate tokenlists */
		tokenlist **tokLists = calloc(PIPE_COUNT + 2, sizeof(tokenlist*));
		int j = 0;
		for (int i = 0; i < PIPE_COUNT + 1; i++) {
			tokLists[i] = new_tokenlist();
			for ( ; j < tokens->size; j++) {
				char* token = tokens->items[j];
				if (token[0] == '|' || token[0] == '&') {
					++j;
					break;
				} else {
					add_token(tokLists[i], tokens->items[j]);
				}
			}
			/* NULLs end of tokenlist */
			tokLists[i]->items = (char**)realloc(tokLists[i]->items, (tokLists[i]->size + 1)*sizeof(char*));
			tokLists[i]->items[tokLists[i]->size] = NULL;
		}

		int p_fds[2];
		int fd_in = 0;
		pid_t pid;
		int numOfCmds = PIPE_COUNT + 1;
		for (int i = 0; i < numOfCmds; i++) { /* execute list of commands, redirect input/output as needed */
			pipe(p_fds);
			if ((pid = fork()) == 0) {
				dup2(fd_in, STDIN_FILENO);
				close(p_fds[0]);
				if (i + 1 < numOfCmds)
					dup2(p_fds[1], STDOUT_FILENO);
				execv(tokLists[i]->items[0], tokLists[i]->items);
				perror("execv");
				exit(1);
			} else if (pid < 0) {
				perror("fork");
				exit(1);
			} else {
				if (NO_WAIT == 0)
					wait(NULL);
				close(p_fds[1]);
				fd_in = p_fds[0];
			}
			if (NO_WAIT == 1 && i == PIPE_COUNT) {
				for (int i = 0; i < 10; i++) {
					if (bgProcesses[i] == 0) {
						bgProcesses[i] = pid;
						printf("[%d]\t%d\n", i+1, bgProcesses[i]);
						break;
					}
				}
			}
		}
		/* close pipes */
		close(p_fds[0]);
		close(p_fds[1]);

		/* free tokLists */
		for (int i = 0; i < PIPE_COUNT + 1; i++) {
			free_tokens(tokLists[i]);
		}
		free(tokLists);

	} else {
		if (NO_WAIT == 1) {
			tokens->items[tokens->size - 1] = NULL;
		}
		int fdIn = -1, fdOut = -1;
		/* open i/o files */
		if (F_IN != NULL) {
			fdIn = open(F_IN, O_RDONLY);
			if (fdIn == -1) {
				printf("%s: No such file or directory\n", F_IN);
				return;
			}
		}
		if (F_OUT != NULL) {
			fdOut = open(F_OUT, O_RDWR | O_CREAT, 0644);
			if (fdOut == -1) {
				printf("%s: Error opening file\n", F_OUT);
				return;
			}
		}

		int pid1 = fork();
		if (pid1 == -1) {
			printf("Unable to fork.\n");
			exit(EXIT_FAILURE);
		} else if (pid1 == 0) {
			/* child process */
			if (fdIn != -1) {
				close(stdin);
				dup2(fdIn, STDIN_FILENO);
			}
			if (fdOut != -1) {
				close(stdout);
				dup2(fdOut, STDOUT_FILENO);
			}
			execv(tokens->items[0], tokens->items);
			exit(0);
		} else {
			/* parent process */
			if (fdIn != -1)
				close(fdIn);
			if (fdOut != -1)
				close(fdOut);
			if (NO_WAIT == 1) {
				for (int i = 0; i < 10; i++) {
					if (bgProcesses[i] == 0) {
						bgProcesses[i] = pid1;
						printf("[%d]\t%d\n", i+1, bgProcesses[i]);
						break;
					}
				}
			} else {
				waitpid(pid1, NULL, 0);
			}
		}
	}
	++CMDS_EXECUTED;
}

void echo(tokenlist *tokens) {
	if (tokens->size > 1) {
		printf("%s", tokens->items[1]);
		for (int i = 2; i < tokens->size; i++) {
			printf(" %s", tokens->items[i]);
		}
		printf("\n");
	}
	++CMDS_EXECUTED;
}

void cd(tokenlist *tokens) {
	if (tokens->size == 1) {
		chdir(getenv("HOME"));
		setenv("PWD", getcwd(NULL, 0), 1); /* sets $PWD to current working directory */
		++CMDS_EXECUTED;
	} else if (tokens->size == 2) {
		int res = chdir(tokens->items[1]);
		if (res == -1) {
			printf("cd: %s: No such file or directory\n", tokens->items[1]);
		} else {
			setenv("PWD", getcwd(NULL, 0), 1);
			++CMDS_EXECUTED;
		}
	} else {
		printf("cd: Too many arguments\n");
	}
}

void jobs(void) {
	for (int i = 0; i < 10; i++) {
		if (bgProcesses[i] != 0) {
			printf("[%d]\t%d\t%s\n", i+1, bgProcesses[i], bgCommands[i]);
		}
	}
	++CMDS_EXECUTED;
}

void exitPrgm(void) {
	waitpid(-1, NULL, 0);
	printf("Commands executed: %i\n", CMDS_EXECUTED);
	exit(EXIT_SUCCESS);
}
