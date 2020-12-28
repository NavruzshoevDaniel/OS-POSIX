//
// Created by Daniel on 28.12.2020.
//
#include <pthread.h>
#include <stdlib.h>
#include "logging.h"

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


void errorPrintf(const char *formatString, ...) {
#ifdef ERROR
    va_list args;
    va_start(args, formatString);
    loggerPrintf("ERROR", RED_COLOR, formatString, args);
    va_end(args);
#endif
}

LOGGER *initLogger() {
    LOGGER *logger = malloc(sizeof(LOGGER));
    logger->error = &errorPrintf;
    logger->info = &infoPrintf;
    logger->warn = &warnPrintf;
    return logger;
}

void warnPrintf(const char *formatString, ...) {
#ifdef WARN
    va_list args;
    va_start(args, formatString);
    loggerPrintf("WARN", YELLOW_COLOR, formatString, args);
    va_end(args);
#endif
}

void infoPrintf(const char *formatString, ...) {
#ifdef INFO
    va_list args;
    va_start(args, formatString);
    loggerPrintf("INFO", GREEN_COLOR, formatString, args);
    va_end(args);
#endif
}

void loggerPrintf(char *level, char *color, const char *formatString, ...) {
#ifdef LOGGING
    va_list args;
    va_start(args, formatString);
    char *clientString;
    vasprintf(&clientString, formatString, args);
    char *curTime = loggerGetTime();
    printf("%s%s - [%d] %s : %s%s\n", color, curTime, pthread_self(), level, clientString, BLACK_COLOR);
    free(curTime);
    free(clientString);
    va_end(args);
#endif
}

LOGGER *closeLogger() {
    LOGGER *logger = malloc(sizeof(LOGGER));
    logger->error = errorPrintf;
    return logger;
}

char *loggerGetTime() {
    time_t tm;
    char *buf;
    struct tm *tm_info;

    if (!(buf = (char *) malloc(sizeof(char) * 9))) {
        return (NULL);
    }
    tm = time(NULL);
    tm_info = localtime(&tm);
    strftime(buf, 9, "%H:%M:%S", tm_info);
    return (buf);
}