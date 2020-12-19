#include <stdio.h>
#include <stdlib.h>

char *CHILD_COLOR = "\x1b[35;1m";
char *PARENT_COLOR = "\x1b[36;1m";
char *ERROR_COLOR = "\x1b[31;1m";
char *DEFAULT_COLOR = "\x1b[37;0m"; 

char *BLACK_COLOR = "\x1b[30m";
char *RED_COLOR = "\x1b[31m";
char *GREEN_COLOR = "\x1b[32m";
char *YELLOW_COLOR = "\x1b[33m";
char *BLUE_COLOR = "\x1b[34m";
char *MAGENTA_COLOR = "\x1b[35m";
char *CYAN_COLOR = "\x1b[36m";
char *WHITE_COLOR = "\x1b[37m";

void throw_and_exit(char *call_name)
{
    fprintf(stderr, "%s\nFailed to %s\n", ERROR_COLOR, call_name);
    perror("");
    exit(EXIT_FAILURE);
}