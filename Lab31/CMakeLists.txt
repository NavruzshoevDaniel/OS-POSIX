cmake_minimum_required(VERSION 3.16)
project(Lab31 C)

set(CMAKE_C_STANDARD 99 )
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")

include_directories("services/")

add_executable(Lab31 main.c services/queue/queueService.c services/queue/queueService.h services/pthread/pthreadService.c services/pthread/pthreadService.h services/threadpool/threadPool.h services/threadpool/threadPool.c argschecker/argsChecker.h argschecker/argsChecker.c services/cache/cache.h services/cache/cache.c services/net/httpService.h services/net/httpService.c services/net/serverSockerService.h services/net/serverSockerService.c config.h main.h services/logger/logging.h services/logger/logger.c services/connection/client/clientConnection.c services/connection/client/clientConnection.h services/connection/server/serverConnection.c services/connection/server/serverConnection.h services/cache/cacheList.h services/cache/cacheList.c services/connection/client/clientConnectionList.h services/connection/client/clientConnectionList.c services/connection/server/serverConnectionList.h services/connection/server/serverConnectionList.c)