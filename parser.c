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
void executeCmd(char*, char*, char**, int);
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


	printf("This is a place holder but should change directory\n");
	return 1;
}

int f_exit(char** argv) {
	return 0;
}


int main()
{


	while (1) {
		printf("%s@%s : %s > ", getenv("USER"), getenv("MACHINE"), getenv("PWD")); //This is the prompt

		/* input contains the whole command
		 * tokens contains substrings from input split by spaces
		 */

		char* input = get_input();
		char* cmdPath = "";
		char* fOutput = "";
		char* fInput = "";
		int builtIn = 0;
		int error = 0;
		//printf("whole input: %s\n", input);
		tokenlist* tokens = get_tokens(input);

		char** argv = malloc(tokens->size * sizeof(char*)); //Create Arguement list size of tokens
		int argI = 0; //Holds index of next free memory space
		for (int i = 0; i < tokens->size; i++) {
			char* token = tokens->items[i];

			//This is the first arguement which is the command
			if (i == 0) {

				if (strcmp(token, "echo") == 0) {
					cmdPath = "echo";
					builtIn = 1;
				}
				else if (strcmp(token, "exit") == 0) {
					cmdPath = "exit";
					builtIn = 1;
				}
				else {
					cmdPath = checkCommand(token);
					if (cmdPath == NULL) {
						break;
					}

				}
				//Free Space, assign value, move memory space forward
				argv[argI] = (char*)malloc(strlen(cmdPath));
				strcpy(argv[argI], cmdPath);
				argI++;
			}

			else if (token[0] == '-') {
				//Free Space, assign value, move memory space forward
				argv[argI] = (char*)malloc(strlen(token));
				strcpy(argv[argI], token);
				argI++;
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

				argv[argI] = (char*)malloc(strlen(token));
				strcpy(argv[argI], token);
				argI++;
				free(cpy);
			}

			else if (token[0] == '~') {
				//Replaces ~ with home environment path
				memmove(token, token + 1, strlen(token));
				char* cpy = (char*)malloc(strlen(token));
				strcpy(cpy, token);
				token = (char*)realloc(token, (strlen(token) + strlen(getenv("HOME")) + 2));
				sprintf(token, "%s%s", getenv("HOME"), cpy);

				argv[argI] = (char*)malloc(strlen(token));
				strcpy(argv[argI], token);
				argI++;

				free(cpy);
			}

			else if (token[0] == '>') {

				fOutput = tokens->items[++i];
			}

			else if (token[0] == '<') {

				fInput = tokens->items[++i];
			}
			else {
				argv[argI] = (char*)malloc(strlen(token));
				strcpy(argv[argI], token);
				argI++;
			}
		}

		argv[argI] = NULL;
		if (builtIn == 0 && error == 0) {
			executeCmd(fOutput, fInput, argv, argI);
		}
		else if (error == 0) {
			for (int i = 0; i < builin_size; i++) {
				if (strcmp(argv[0], builtin_str[i]) == 0) {
					int res = (*builtin_func[i])(argv);
					if (res == 0)
						return 0;
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

	printf("Command not found\n");
	return NULL;


}

void executeCmd(char* output, char* input, char** argv, int argI) {
	int pid = fork();

	if (pid == 0) {
		/*This is a child proccess*/

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


		int result = execv(argv[0], argv);


		exit(0);

	}
	else {
		waitpid(pid, NULL, 0); //Wait for child process to finish
	}


}