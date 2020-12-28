cmake_minimum_required(VERSION 3.16)
project(Lab31 C)

set(CMAKE_C_STANDARD 99 )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")


add_executable(Lab31 main.c services/queue/queueService.c services/queue/queueService.h services/pthread/pthreadService.c services/pthread/pthreadService.h services/threadpool/threadPool.h services/threadpool/threadPool.c argschecker/argsChecker.h argschecker/argsChecker.c services/connection/connection.h services/connection/connection.c services/concurrent/atomicInt.h services/concurrent/atomicInt.c services/cache/cache.h services/cache/cache.c services/net/httpService.h services/net/httpService.c services/proxyhandlers/getRequest/getRequestHandler.h services/proxyhandlers/getRequest/getRequestHandler.c services/proxyhandlers/writeToServer/writeToServerHandler.h services/proxyhandlers/writeToServer/writeToServerHandler.c services/proxyhandlers/readFromServerWriteClient/readFromServerWriteToClientHandler.h services/proxyhandlers/readFromServerWriteClient/readFromServerWriteToClientHandler.h.c services/proxyhandlers/readFromCacheWriteToClient/readFromCacheWriteToClientState.c services/proxyhandlers/readFromCacheWriteToClient/readFromCacheWriteToClientState.h services/net/serverSockerService.h services/net/serverSockerService.c config.h main.h services/logger/logging.h services/logger/logger.c)
add_executable(Test test/loggerTest.c services/logger/logger.c)