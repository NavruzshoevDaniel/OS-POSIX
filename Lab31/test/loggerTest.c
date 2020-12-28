//
// Created by Daniel on 28.12.2020.
//
#include "../services/logger/logging.h"

int main(int argc, const char *argv[]) {
    LOGGER *logger = initLogger();
    logger->info("halo");
}
