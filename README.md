# OS_Implement_Shell

[Link to GitHub](https://github.com/NickTidwell/OS_Implement_Shell.git)

## Contents of Tar File:

* makefile
* parser.c
* README.md

## How to compile executable:

### Input these commands in your terminal in the directory where the extracted tar file contents are:

    make

### This should produce a "shell" executable that you can run by typing:

    ./shell


## Project Statement

### Design and implement a basic shell interface that supports input/output redirection, piping background processing, and a series of built in functions (echo, cd, exit, jobs).

## Assumptions Made

* Error messages do not need to match the exact wording of bash
* Does not handle globs, regular expressions, special characters (except &, <, >, |, ~, and $), quotes, escaped characters, etc.
* No more than 10 background processes at the same time
* Multiple redirections of the same type will not appear in a single command
* Auto-complete not implemented
* Only expands environment variables given as whole arguments
* Piping and I/O redirection will not occur together in a single command
* No more than two pipes (|) will appear in a single command

## Contributions by Group Member

### Nicholas Tidwell

* implemented Environmental Variables
* implemented Prompt
* implemented Tilde Expansion
* implemented $PATH search
* implemented External Command Execution
* implemented i/o redirection
* implemented piping
* implemented 'cd'

### William Tsaur

* created README.md
* created makefile
* modified environment variable parsing
* modified $PATH search implementation
* implemented External Command Execution
* modified i/o redirection
* modified piping
* implemented 'exit'
* modified 'cd'

### Michael Styron

* implemented background processing
* implemented 'jobs'
* testing
* debugging

## Extra Credit (task - implemented by)

* Shell-ception - Group
* Unlimited Piping - William Tsaur
