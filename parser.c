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

int isBuiltIn(char* cmd);
void echo(tokenlist *tokens);
void cd(tokenlist *tokens);
void exit();
void jobs();

//static int NUM_OF_BUILT_INS = 3;
//static char** BUILT_INS = {"echo", "cd", "jobs"}; //list of built-in functions besides "exit"

static int cmdExecutions = 0;

static char* F_OUT;	//variables to store name of files for i/o redirection
static char* F_IN;

int main()
{
	while (1) {
		F_OUT = NULL;
		F_IN = NULL;

		printf("%s@%s : %s > ", getenv("USER"), getenv("MACHINE"), getenv("PWD")); //This is the prompt

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
//printf("whole input: %s\n", input);
		tokenlist *tokens = get_tokens(input, " ");

		// checks to see if command is a valid command or if command == "exit"
		char *cmd = tokens->items[0];
		if (strcmp(cmd, "exit") == 0) {		//built-in function: waits for all processes to finish then terminates program
            waitpid(-1, NULL, 0);
			printf("Commands executed: %i\n", cmdExecutions);
			exit(EXIT_SUCCESS);
		}
		char* cmdPath = cmdSearch(cmd);
		if (cmdPath == NULL) continue;
		tokens->items[0] = cmdPath;
//printf("token 0: (%s)\n", tokens->items[0]);
		
		//command parsing - expanding varaibles
		for (int i = 1; i < tokens->size; i++) {
			char* token = tokens->items[i];

			//This Check is To see if token is environment variable
			if (token[0] == '$') {
				memmove(token, token + 1, strlen(token)); //Moves pointer forward trimming off $
				char* envVar = getenv(token);  //Used to check if token is an existing environment variable
				if (envVar != NULL) {
					tokens->items[i] = (char*)realloc(tokens->items[i], strlen(envVar) + 1); //Allocate New Space for token
				} else {
                    tokens->items[i] = (char*)realloc(tokens->items[i], 1);
					envVar = "\0";  //token was not an existing environment variable
				}
				strcpy(tokens->items[i], envVar); //Assign token to enveronment variable
			} else if (token[0] == '~') {
				//Replaces ~ with home environment path
				memmove(token, token + 1, strlen(token));
				char* cpy = (char*)malloc(strlen(token) + 1);
				strcpy(cpy, token);
				token = (char*)realloc(token, (strlen(token) + strlen(getenv("HOME")) + 2));
				sprintf(token, "%s%s", getenv("HOME"), cpy);
			} else if (token[0] == '<') {
				if (++i < tokens->size)
					F_IN = tokens->items[i];
			} else if (token [0] == '>') {
				if (++i < tokens->size)
					F_OUT = tokens->items[i];
			}
//printf("token %d: (%s)\n", i, tokens->items[i]);
		}
		//executing command
		cmdExecute(tokens);

        free(input);
		free_tokens(tokens);
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
	tokenlist *pathTokens = get_tokens(path, ":");
	char *cmdToAppend = (char*)malloc(strlen(cmd) + 1);
	strcpy(cmdToAppend, "/");
	strcat(cmdToAppend, cmd);
	for (int i = 0; i < pathTokens->size; i++) {
		char *token = pathTokens->items[i];
		token = (char*)realloc(token, strlen(cmdToAppend) + strlen(token) + 1);
		strcat(token, cmdToAppend);
		if (access(token, F_OK) == 0) return token;
	}
	printf("%s: command not found\n", cmd);
    free_tokens(pathTokens);
    free(cmdToAppend);
    free(path);
	return NULL;
}

void cmdExecute(tokenlist *tokens) {
	int oldIn = dup(0);
	int oldOut = dup(1);
	int fdIn, fdOut;
	if (F_IN != NULL) {
		if (access(F_IN, F_OK) == 0) {
			fdIn = open(F_IN, O_RDONLY);
		}
	} else {
		fdIn = dup(oldIn);
	}

	pid_t child_pid = fork();
    //int status;
    if (child_pid == -1) {
        printf("Unable to fork.");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
		execv(tokens->items[0], tokens->items);
        exit(0);
	} else {
        waitpid(child_pid, NULL, 0);
	    ++cmdExecutions;
    }
}

// int isBuiltIn(char* cmd) {
// 	if (cmd != NULL) {
// 		for (int i = 0; i < NUM_OF_BUILT_INS; i++) {
// 			if (strcmp(cmd, BUILT_INS[i]) == 0) {
// 				return i;
// 			}
// 		}
// 	}
// 	return -1;
// }

void echo(tokenlist *tokens) {
	if (tokens->size > 1) {
		printf("%s", tokens->items[1]);
		for (int i = 2; i < tokens->size; i++) {
			printf(" %s", tokens->items[i]);
		}
		printf("\n");
	}
}

void cd(tokenlist *tokens) {
	if (tokens->size == 1) {
		chdir(getenv("HOME"));
	} else if (tokens->size == 2) {
		char* tok = calloc(strlen(getenv("PWD")), sizeof(char));
		strcpy(tok, getenv("PWD"));
		strcat(tok, tokens->items[1]);
		if (access(tok, F_OK) == 0) {
			struct stat buf;
			stat(tok, &buf);
			if (S_ISDIR(buf.st_mode)) {
				chdir(tok);
			} else {
				printf("cd: %s: Not a directory\n", tokens->items[1]);
			}
		} else {
			printf("cd: %s: No such file or directory\n", tokens->items[1]);
		}
		free(tok);
	} else {
		printf("cd: too many arguments\n");
	}
}

void jobs() {

}
