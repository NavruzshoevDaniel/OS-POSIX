//
// Created by Daniel on 19.12.2020.
//
#include "argsChecker.h"

void checkCountArguments(int argc) {
    if (argc != 2) {
        perror("Wrong count of arguments");
        exit(EXIT_FAILURE);
    }
}

void checkIfValidParsedInt(int number) {
    if (number <= 0) {
        fprintf(stderr, "Incorrect input\n");
        exit(EXIT_FAILURE);
    }
}