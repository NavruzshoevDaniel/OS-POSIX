cmake_minimum_required(VERSION 3.16)
project(Lab31 C)

set(CMAKE_C_STANDARD 99)

add_executable(Lab31 main.c services/queue/queueService.c services/queue/queueService.h services/pthread/pthreadService.c services/pthread/pthreadService.h services/threadpool/threadPool.h services/threadpool/threadPool.c argschecker/argsChecker.h argschecker/argsChecker.c services/connection/connection.h services/connection/connection.c services/concurrent/atomicInt.h services/concurrent/atomicInt.c)