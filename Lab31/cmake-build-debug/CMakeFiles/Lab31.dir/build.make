# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = "/Users/daniel/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/201.7846.88/CLion.app/Contents/bin/cmake/mac/bin/cmake"

# The command to remove a file.
RM = "/Users/daniel/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/201.7846.88/CLion.app/Contents/bin/cmake/mac/bin/cmake" -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31"

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug"

# Include any dependencies generated for this target.
include CMakeFiles/Lab31.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/Lab31.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Lab31.dir/flags.make

CMakeFiles/Lab31.dir/main.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/main.c.o: ../main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/Lab31.dir/main.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/main.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/main.c"

CMakeFiles/Lab31.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/main.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/main.c" > CMakeFiles/Lab31.dir/main.c.i

CMakeFiles/Lab31.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/main.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/main.c" -o CMakeFiles/Lab31.dir/main.c.s

CMakeFiles/Lab31.dir/services/queue/queueService.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/queue/queueService.c.o: ../services/queue/queueService.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/Lab31.dir/services/queue/queueService.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/queue/queueService.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/queue/queueService.c"

CMakeFiles/Lab31.dir/services/queue/queueService.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/queue/queueService.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/queue/queueService.c" > CMakeFiles/Lab31.dir/services/queue/queueService.c.i

CMakeFiles/Lab31.dir/services/queue/queueService.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/queue/queueService.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/queue/queueService.c" -o CMakeFiles/Lab31.dir/services/queue/queueService.c.s

CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.o: ../services/pthread/pthreadService.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_3) "Building C object CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/pthread/pthreadService.c"

CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/pthread/pthreadService.c" > CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.i

CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/pthread/pthreadService.c" -o CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.s

CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.o: ../services/threadpool/threadPool.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_4) "Building C object CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/threadpool/threadPool.c"

CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/threadpool/threadPool.c" > CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.i

CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/threadpool/threadPool.c" -o CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.s

CMakeFiles/Lab31.dir/argschecker/argsChecker.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/argschecker/argsChecker.c.o: ../argschecker/argsChecker.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_5) "Building C object CMakeFiles/Lab31.dir/argschecker/argsChecker.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/argschecker/argsChecker.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/argschecker/argsChecker.c"

CMakeFiles/Lab31.dir/argschecker/argsChecker.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/argschecker/argsChecker.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/argschecker/argsChecker.c" > CMakeFiles/Lab31.dir/argschecker/argsChecker.c.i

CMakeFiles/Lab31.dir/argschecker/argsChecker.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/argschecker/argsChecker.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/argschecker/argsChecker.c" -o CMakeFiles/Lab31.dir/argschecker/argsChecker.c.s

CMakeFiles/Lab31.dir/services/connection/connection.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/connection/connection.c.o: ../services/connection/connection.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_6) "Building C object CMakeFiles/Lab31.dir/services/connection/connection.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/connection/connection.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/connection/connection.c"

CMakeFiles/Lab31.dir/services/connection/connection.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/connection/connection.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/connection/connection.c" > CMakeFiles/Lab31.dir/services/connection/connection.c.i

CMakeFiles/Lab31.dir/services/connection/connection.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/connection/connection.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/connection/connection.c" -o CMakeFiles/Lab31.dir/services/connection/connection.c.s

CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.o: ../services/concurrent/atomicInt.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_7) "Building C object CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/concurrent/atomicInt.c"

CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/concurrent/atomicInt.c" > CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.i

CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/concurrent/atomicInt.c" -o CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.s

CMakeFiles/Lab31.dir/services/cache/cache.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/cache/cache.c.o: ../services/cache/cache.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_8) "Building C object CMakeFiles/Lab31.dir/services/cache/cache.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/cache/cache.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/cache/cache.c"

CMakeFiles/Lab31.dir/services/cache/cache.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/cache/cache.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/cache/cache.c" > CMakeFiles/Lab31.dir/services/cache/cache.c.i

CMakeFiles/Lab31.dir/services/cache/cache.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/cache/cache.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/cache/cache.c" -o CMakeFiles/Lab31.dir/services/cache/cache.c.s

CMakeFiles/Lab31.dir/services/http/httpSrvice.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/http/httpSrvice.c.o: ../services/http/httpSrvice.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_9) "Building C object CMakeFiles/Lab31.dir/services/http/httpSrvice.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/http/httpSrvice.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/http/httpSrvice.c"

CMakeFiles/Lab31.dir/services/http/httpSrvice.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/http/httpSrvice.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/http/httpSrvice.c" > CMakeFiles/Lab31.dir/services/http/httpSrvice.c.i

CMakeFiles/Lab31.dir/services/http/httpSrvice.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/http/httpSrvice.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/http/httpSrvice.c" -o CMakeFiles/Lab31.dir/services/http/httpSrvice.c.s

CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.o: ../services/proxyhandlers/getRequest/getRequestHandler.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_10) "Building C object CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/proxyhandlers/getRequest/getRequestHandler.c"

CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/proxyhandlers/getRequest/getRequestHandler.c" > CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.i

CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/proxyhandlers/getRequest/getRequestHandler.c" -o CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.s

CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.o: CMakeFiles/Lab31.dir/flags.make
CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.o: ../services/proxyhandlers/writeToServer/writeToServerHandler.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_11) "Building C object CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.o"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.o   -c "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/proxyhandlers/writeToServer/writeToServerHandler.c"

CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.i"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/proxyhandlers/writeToServer/writeToServerHandler.c" > CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.i

CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.s"
	/Library/Developer/CommandLineTools/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/services/proxyhandlers/writeToServer/writeToServerHandler.c" -o CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.s

# Object files for target Lab31
Lab31_OBJECTS = \
"CMakeFiles/Lab31.dir/main.c.o" \
"CMakeFiles/Lab31.dir/services/queue/queueService.c.o" \
"CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.o" \
"CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.o" \
"CMakeFiles/Lab31.dir/argschecker/argsChecker.c.o" \
"CMakeFiles/Lab31.dir/services/connection/connection.c.o" \
"CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.o" \
"CMakeFiles/Lab31.dir/services/cache/cache.c.o" \
"CMakeFiles/Lab31.dir/services/http/httpSrvice.c.o" \
"CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.o" \
"CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.o"

# External object files for target Lab31
Lab31_EXTERNAL_OBJECTS =

Lab31: CMakeFiles/Lab31.dir/main.c.o
Lab31: CMakeFiles/Lab31.dir/services/queue/queueService.c.o
Lab31: CMakeFiles/Lab31.dir/services/pthread/pthreadService.c.o
Lab31: CMakeFiles/Lab31.dir/services/threadpool/threadPool.c.o
Lab31: CMakeFiles/Lab31.dir/argschecker/argsChecker.c.o
Lab31: CMakeFiles/Lab31.dir/services/connection/connection.c.o
Lab31: CMakeFiles/Lab31.dir/services/concurrent/atomicInt.c.o
Lab31: CMakeFiles/Lab31.dir/services/cache/cache.c.o
Lab31: CMakeFiles/Lab31.dir/services/http/httpSrvice.c.o
Lab31: CMakeFiles/Lab31.dir/services/proxyhandlers/getRequest/getRequestHandler.c.o
Lab31: CMakeFiles/Lab31.dir/services/proxyhandlers/writeToServer/writeToServerHandler.c.o
Lab31: CMakeFiles/Lab31.dir/build.make
Lab31: CMakeFiles/Lab31.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir="/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles" --progress-num=$(CMAKE_PROGRESS_12) "Linking C executable Lab31"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Lab31.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Lab31.dir/build: Lab31

.PHONY : CMakeFiles/Lab31.dir/build

CMakeFiles/Lab31.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Lab31.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Lab31.dir/clean

CMakeFiles/Lab31.dir/depend:
	cd "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug" && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31" "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31" "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug" "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug" "/Users/daniel/NSU/Semestr 5/OS-POSIX/Lab31/cmake-build-debug/CMakeFiles/Lab31.dir/DependInfo.cmake" --color=$(COLOR)
.PHONY : CMakeFiles/Lab31.dir/depend

