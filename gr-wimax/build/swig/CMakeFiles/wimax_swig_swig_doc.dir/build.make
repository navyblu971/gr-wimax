# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.5

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/atexide/gr-wimax/gr-wimax

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/atexide/gr-wimax/gr-wimax/build

# Utility rule file for wimax_swig_swig_doc.

# Include the progress variables for this target.
include swig/CMakeFiles/wimax_swig_swig_doc.dir/progress.make

swig/CMakeFiles/wimax_swig_swig_doc: swig/wimax_swig_doc.i


swig/wimax_swig_doc.i: swig/wimax_swig_doc_swig_docs/xml/index.xml
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/atexide/gr-wimax/gr-wimax/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Generating python docstrings for wimax_swig_doc"
	cd /home/atexide/gr-wimax/gr-wimax/docs/doxygen && /usr/bin/python2 -B /home/atexide/gr-wimax/gr-wimax/docs/doxygen/swig_doc.py /home/atexide/gr-wimax/gr-wimax/build/swig/wimax_swig_doc_swig_docs/xml /home/atexide/gr-wimax/gr-wimax/build/swig/wimax_swig_doc.i

swig/wimax_swig_doc_swig_docs/xml/index.xml: swig/_wimax_swig_doc_tag
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/atexide/gr-wimax/gr-wimax/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Generating doxygen xml for wimax_swig_doc docs"
	cd /home/atexide/gr-wimax/gr-wimax/build/swig && ./_wimax_swig_doc_tag
	cd /home/atexide/gr-wimax/gr-wimax/build/swig && /usr/bin/doxygen /home/atexide/gr-wimax/gr-wimax/build/swig/wimax_swig_doc_swig_docs/Doxyfile

wimax_swig_swig_doc: swig/CMakeFiles/wimax_swig_swig_doc
wimax_swig_swig_doc: swig/wimax_swig_doc.i
wimax_swig_swig_doc: swig/wimax_swig_doc_swig_docs/xml/index.xml
wimax_swig_swig_doc: swig/CMakeFiles/wimax_swig_swig_doc.dir/build.make

.PHONY : wimax_swig_swig_doc

# Rule to build all files generated by this target.
swig/CMakeFiles/wimax_swig_swig_doc.dir/build: wimax_swig_swig_doc

.PHONY : swig/CMakeFiles/wimax_swig_swig_doc.dir/build

swig/CMakeFiles/wimax_swig_swig_doc.dir/clean:
	cd /home/atexide/gr-wimax/gr-wimax/build/swig && $(CMAKE_COMMAND) -P CMakeFiles/wimax_swig_swig_doc.dir/cmake_clean.cmake
.PHONY : swig/CMakeFiles/wimax_swig_swig_doc.dir/clean

swig/CMakeFiles/wimax_swig_swig_doc.dir/depend:
	cd /home/atexide/gr-wimax/gr-wimax/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/atexide/gr-wimax/gr-wimax /home/atexide/gr-wimax/gr-wimax/swig /home/atexide/gr-wimax/gr-wimax/build /home/atexide/gr-wimax/gr-wimax/build/swig /home/atexide/gr-wimax/gr-wimax/build/swig/CMakeFiles/wimax_swig_swig_doc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : swig/CMakeFiles/wimax_swig_swig_doc.dir/depend

