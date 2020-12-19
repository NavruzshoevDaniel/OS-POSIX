#include "pthread.h"
#include "sys/stat.h"
#include "dirent.h"
#include "stddef.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "fcntl.h"

#define ERROR_CODE -1
#define FILE_BUFFER_SIZE 512
#define SIZE_LENGTHS 2

size_t direntLen;

void *cpFunction(void *arg);

int copyFolder(const char *sourcePath, const char *destinationPath, mode_t mode);

char **buildNewPath(const char *sourcePath, const char *destinationPath, const char *additionalPath);

void freeCharsets(char **charSets, size_t lengthsSize);

char **parseArguments(char **args);

int isValidArguments(int argc, char **args);

char **allocateCharsets(const size_t *lengths, size_t lengthsSize);

int main(int argc, char *argv[]) {
    if (isValidArguments(argc, argv) != EXIT_SUCCESS) {
        pthread_exit((void *) 0);
    }
    char **sourceAndDestinationPaths = parseArguments(&argv[1]);

    if (sourceAndDestinationPaths == NULL) {
        pthread_exit((void *) ERROR_CODE);
    }

    ssize_t pathlen = pathconf(sourceAndDestinationPaths[0], _PC_NAME_MAX);
    pathlen = (pathlen == -1 ? 255 : pathlen);
    direntLen = offsetof(struct dirent, d_name) + pathlen + 1;

    cpFunction(sourceAndDestinationPaths);
    pthread_exit((void *) 0);
}

char **allocateCharsets(const size_t *lengths, size_t lengthsSize) {
    char **charsets = (char **) malloc(sizeof(char *) * lengthsSize);
    for (int i = 0; i < lengthsSize; ++i) {
        charsets[i] = (char *) malloc(sizeof(char) * lengths[i]);
    }
    return charsets;
}

void freeCharsets(char **charSets, size_t lengthsSize) {
    for (int i = 0; i < lengthsSize; ++i) {
        free(charSets[i]);
    }
    free(charSets);
}

char **buildNewPath(const char *sourcePath, const char *destinationPath, const char *additionalPath) {
    char **result;
    size_t additionalLen = strlen(additionalPath);
    size_t sourcePathLen = strlen(sourcePath) + additionalLen + 1;
    size_t destinationPathLen = strlen(destinationPath) + additionalLen + 1;

    size_t lengths[SIZE_LENGTHS] = {sourcePathLen, destinationPathLen};

    result = allocateCharsets(lengths, SIZE_LENGTHS);

    strcpy(result[0], sourcePath);
    strcat(result[0], "/");
    strcat(result[0], additionalPath);

    strcpy(result[1], destinationPath);
    strcat(result[1], "/");
    strcat(result[1], additionalPath);
    return result;
}

int copyFolder(const char *sourcePath, const char *destinationPath, mode_t mode) {
    DIR *dir;
    struct dirent *entry, *result;
    if (mkdir(destinationPath, mode) == ERROR_CODE && errno != EEXIST) {
        fprintf(stderr, "Couldn't create directory %s, %s\n", destinationPath, strerror(errno));
        return ERROR_CODE;
    }

    while ((dir = opendir(sourcePath)) == NULL) {
        if (errno != EMFILE) {
            printf("Couldn't open directory %s, %s\n", sourcePath, strerror(errno));
            return ERROR_CODE;
        }
    }
    entry = (struct dirent *) malloc(direntLen);
    if (entry == NULL) {
        perror(strerror(errno));
    }
    while (readdir_r(dir, entry, &result) == 0 && result != NULL) {
        pthread_t thread;
        char **newPaths;
        int status;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        newPaths = buildNewPath(sourcePath, destinationPath, entry->d_name);
        do {
            status = pthread_create(&thread, NULL, cpFunction, (void *) newPaths);
        } while (status != 0 && errno == EAGAIN);
        if (status != 0) {
            fprintf(stderr, "Couldn't copy %s, %s\n", newPaths[0], strerror(errno));
            freeCharsets(newPaths, SIZE_LENGTHS);
        }
    }
    free(entry);
    if (closedir(dir) == ERROR_CODE) {
        fprintf(stderr, "Couldn't close directory, %s\n", strerror(errno));
    }


    return EXIT_SUCCESS;
}

int copyFile(const char *sourcePath, const char *destinationPath, mode_t mode) {
    static const int OPEN_FILE_FLAGS = O_WRONLY | O_CREAT | O_EXCL;
    int fdin, fdout;
    int bytesRead;
    char buffer[FILE_BUFFER_SIZE];
    int returnValue = EXIT_SUCCESS;

    while ((fdin = open(sourcePath, O_RDONLY)) == -1) {
        if (errno != EMFILE) {
            fprintf(stderr, "Couldn't open file %s, %s\n", sourcePath, strerror(errno));
            return ERROR_CODE;
        }
    }
    while ((fdout = open(destinationPath, OPEN_FILE_FLAGS, mode)) == -1) {
        if (errno != EMFILE) {
            fprintf(stderr, "Couldn't open file %s, %s\n", destinationPath, strerror(errno));
            if (close(fdin) != 0)
                perror(strerror(errno));
            return ERROR_CODE;
        }
    }
    while ((bytesRead = read(fdin, buffer, FILE_BUFFER_SIZE)) > 0 || errno == EINTR) {
        char *writePtr = buffer;
        int bytesWritten;
        do {
            bytesWritten = write(fdout, writePtr, bytesRead);
            if (bytesWritten >= 0) {
                bytesRead -= bytesWritten;
                writePtr += bytesWritten;
            } else if (errno != EINTR) {
                perror(strerror(errno));
                returnValue = ERROR_CODE;
                break;
            }
        } while (bytesRead > 0);
    }
    if (bytesRead < 0) {
        perror(strerror(errno));
        returnValue = ERROR_CODE;
    }
    if (close(fdin) != 0) {
        perror(strerror(errno));
        returnValue = ERROR_CODE;
    }
    if (close(fdout) != 0) {
        perror(strerror(errno));
        returnValue = ERROR_CODE;
    }
    return returnValue;
}

void *cpFunction(void *arg) {
    struct stat statBuffer;
    char *sourcePath = ((char **) arg)[0];
    char *destinationPath = ((char **) arg)[1];

    if (stat(sourcePath, &statBuffer) != 0) {
        fprintf(stderr, "%s, %s\n", strerror(errno));
        freeCharsets(arg, SIZE_LENGTHS);
        return (void *) ERROR_CODE;
    }
    if (S_ISDIR(statBuffer.st_mode)) {
        copyFolder(sourcePath, destinationPath, statBuffer.st_mode);
    } else if (S_ISREG(statBuffer.st_mode)) {
        copyFile(sourcePath, destinationPath, statBuffer.st_mode);
    }

    freeCharsets(arg, SIZE_LENGTHS);
}

int isValidArguments(int argc, char **args) {
    struct stat buffer;

    if (argc < 3) {
        printf("usage %s <copy source> <copy destination>\n", args[0]);
        return ERROR_CODE;
    }

    if (stat(args[1], &buffer) == -1) {
        fprintf(stderr, "Bad argument: %s - %s\n", args[1], strerror(errno));
        return ERROR_CODE;
    }
    return EXIT_SUCCESS;
}

char **parseArguments(char **args) {

    size_t len[2] = {strlen(args[0]), strlen(args[1])};

    char **result = allocateCharsets(len, SIZE_LENGTHS);
    for (int i = 0; i < 2; i++) {
        strcpy(result[i], args[i]);
        if (result[i][len[i] - 1] == '/') {
            result[i][len[i] - 1] = 0;
        }
    }
    return result;
}