# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

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
CMAKE_COMMAND = "/Users/elin/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/182.4505.18/CLion.app/Contents/bin/cmake/mac/bin/cmake"

# The command to remove a file.
RM = "/Users/elin/Library/Application Support/JetBrains/Toolbox/apps/CLion/ch-0/182.4505.18/CLion.app/Contents/bin/cmake/mac/bin/cmake" -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/elin/cs/os161-1.99

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/elin/cs/os161-1.99/cmake-build-debug

# Utility rule file for ASST2-OPT.

# Include the progress variables for this target.
include CMakeFiles/ASST2-OPT.dir/progress.make

CMakeFiles/ASST2-OPT:
	cd /Users/elin/cs/os161-1.99/cmake-build-debug/kern/compile/ASST2-OPT && ./config ASST2-OPT
	cd /Users/elin/cs/os161-1.99/cmake-build-debug/kern/compile/ASST2-OPT && bmake depend
	cd /Users/elin/cs/os161-1.99/cmake-build-debug/kern/compile/ASST2-OPT && bmake
	cd /Users/elin/cs/os161-1.99/cmake-build-debug/kern/compile/ASST2-OPT && bmake install

ASST2-OPT: CMakeFiles/ASST2-OPT
ASST2-OPT: CMakeFiles/ASST2-OPT.dir/build.make

.PHONY : ASST2-OPT

# Rule to build all files generated by this target.
CMakeFiles/ASST2-OPT.dir/build: ASST2-OPT

.PHONY : CMakeFiles/ASST2-OPT.dir/build

CMakeFiles/ASST2-OPT.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ASST2-OPT.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ASST2-OPT.dir/clean

CMakeFiles/ASST2-OPT.dir/depend:
	cd /Users/elin/cs/os161-1.99/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/elin/cs/os161-1.99 /Users/elin/cs/os161-1.99 /Users/elin/cs/os161-1.99/cmake-build-debug /Users/elin/cs/os161-1.99/cmake-build-debug /Users/elin/cs/os161-1.99/cmake-build-debug/CMakeFiles/ASST2-OPT.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ASST2-OPT.dir/depend

