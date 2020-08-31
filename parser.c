#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct {
	int size;
	char **items;
} tokenlist;

char *get_input(void);
tokenlist *get_tokens(char *input, char *delims);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);
char* cmdSearch(char *cmd);
void cmdExecute(tokenlist *tokens);

static int cmdExecutions = 0;

int main()
{
	while (1) {
		printf("%s@%s : %s > ", getenv("USER"), getenv("MACHINE"), getenv("PWD")); //This is the prompt

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		printf("whole input: %s\n", input);
		tokenlist *tokens = get_tokens(input, " ");

		// checks to see if command is a valid command or if command == "exit"
		char *cmd = tokens->items[0];
		if (strcmp(cmd, "exit") == 0) {
			//exit implementation not finished!
			printf("Commands executed: %i\n", cmdExecutions);
			break;
		}

printf("Calling cmdSearch. cmd = %s\n", cmd);

		cmdSearch(cmd);

printf("Executed cmdSearch\n");

		char* cmdPath = cmdSearch(cmd);
		if (cmdPath == NULL) continue;
		tokens->items[0] = cmdPath;

		//command parsing - expanding varaibles
		for (int i = 1; i < tokens->size; i++) {
			char * token = (char *) malloc(strlen(tokens->items[i]));
			strcpy(token, tokens->items[i]);
			//char* token = tokens->items[i];

			//This Check is To see if token is environment variable
			if (token[0] == '$') {
				memmove(token, token + 1, strlen(token)); //Moves pointer forward trimming off $
				char* envVar = getenv(token);  //Used to check if token is an existing environment variable
				if (envVar != NULL) {
					token = (char*)realloc(token, strlen(envVar)); //Allocate New Space for token
				} else {
					envVar = "";  //token was not an existing environment variable
				}
				strcpy(tokens->items[i], envVar); //Assign token to enveronment variable
			} else if (token[0] == '~') {
				//Replaces ~ with home environment path
				memmove(token, token + 1, strlen(token));
				char* cpy = (char*)malloc(strlen(token));
				strcpy(cpy, token);
				token = (char*)realloc(token, (strlen(token) + strlen(getenv("HOME")) + 2));
				sprintf(token, "%s%s", getenv("HOME"), cpy);
			}
			free(token);
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}

		//executing command
		cmdExecute(tokens);
		++cmdExecutions;

		free(input);
		free_tokens(tokens);
	}

	return 0;
}

void cmdExecute(tokenlist *tokens) {
	pid_t child_pid = fork();
	int status;
	if (child_pid == 0) {
		execv(tokens->items[0], tokens->items);
	} else {
		wait(&status);
	}
}

char* cmdSearch (char *cmd) {
printf("Executing cmdSearch\n");
	tokenlist *pathTokens = get_tokens(getenv("PATH"), ":");
	char *cmdToAppend = (char*)malloc(strlen(cmd) + 1);
	cmdToAppend[0] = '/';
	strcat(cmdToAppend, cmd);
	for (int i = 0; i < pathTokens->size; i++) {
		char * token = (char *) malloc(strlen(pathTokens->items[i]));
		strcpy(token, pathTokens->items[i]);
		token = (char*)realloc(token, strlen(cmdToAppend));
		strcat(token, cmdToAppend);
		if (access(token, F_OK) == 0) return token;
		free(token);
	}
	printf("%s: command not found\n", cmd);
	free_tokens(pathTokens);
	free(cmdToAppend);
	return NULL;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = NULL;
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
printf("entered add_token: %s\n", item);
	int i = tokens->size;
printf("checking strlen before realloc: %li\n", strlen(item));
printf("tokens->size = %i\n", i);
printf("tokens->items address: %p\n", tokens->items);
	if (tokens->items == NULL) {
		tokens->items = malloc((i + 1) * sizeof(char*));
printf("malloc executed\n");
	} else 
		tokens->items = (char **) realloc(tokens->items, (i + 1) * sizeof(char*));
printf("tokens->items mem reallocated\n");
printf("checking strlen after realloc: %li\n", strlen(item));
printf("tokens->size = %i, i = %i\n", tokens->size, i);
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
printf("token->items[i] mem allocated\n");
	strcpy(tokens->items[i], item);
printf("copied item --> token->items[i]\n");
	tokens->size += 1;
printf("finished adding token\n");
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

tokenlist *get_tokens(char *input, char* delims)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);
	tokenlist *tokens = new_tokenlist();
	char *tok = strtok(buf, delims);
	while (tok != NULL) {
printf("While loop entered, strlen(tok) = %li\n", strlen(tok));
		add_token(tokens, tok);
printf("token added\n");
		tok = strtok(NULL, delims);
	}

	free(buf);
printf("get_tokens() finished executing\n");
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
}
