#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int size;
	char **items;
} tokenlist;

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

int main()
{
	while (1) {
		printf("%s@%s : %s > ", getenv("USER"), getenv("MACHINE"), getenv("PWD")); //This is the prompt

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char *input = get_input();
		printf("whole input: %s\n", input);
		tokenlist *tokens = get_tokens(input);
		for (int i = 0; i < tokens->size; i++) {
			char* token = tokens->items[i];

			//This Check is To see if token is environment variable
			if (token[0] == '$') {
				memmove(tokens->items[i], tokens->items[i] + 1, strlen(tokens->items[i])); //Moves pointer forward trimming off $
				token = (char*)realloc(token, strlen(getenv(token))); //Allocate New Space for token
				strcpy(tokens->items[i], getenv(token)); //Assign token to enveronment variable
			}

			if (token[0] == '~') {
				//Replaces ~ with home environment path
				memmove(token, token + 1, strlen(token));
				char* cpy = (char*)malloc(strlen(token));
				strcpy(cpy, token);
				token = (char*)realloc(token, (strlen(token) + strlen(getenv("HOME")) + 2));
				sprintf(token, "%s%s", getenv("HOME"), cpy);
			}

			printf("token %d: (%s)\n", i, tokens->items[i]);
		}
		
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

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **) realloc(tokens->items, (i + 1) * sizeof(char *));
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
	strcpy(tokens->items[i], item);

	tokens->size += 1;
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

tokenlist *get_tokens(char *input)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);

	free(tokens);
}
