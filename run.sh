#!/bin/bash

# Functions
function compile_normal {
	cmake -DCMAKE_BUILD_TYPE=Release .

	make -j8 $1
}

function compile_debug {
	mkdir -p debug

	cmake -DCMAKE_BUILD_TYPE=Debug .

	make -j8 $1

	mv $1 debug/$1-debug
}

function czhp_normal {
	set +e

	mkdir -p bin

	# Compile the library
	echo "==========================================================="
	echo "Testing single file library compilation..."
	echo -e "===========================================================\n"

	./debug/czhp-run -c samples/zhp/zhp_library.cpp

	ls -la samples/zhp/*.zhplib

	echo -e "\n==========================================================="
	echo "Testing separate file library compilation..."
	echo -e "===========================================================\n"

	./debug/czhp-run -c samples/zhp/l1.cpp samples/zhp/l2.cpp

	ls -la samples/zhp/*.zhplib

	echo -e "\n==========================================================="
	echo "Testing separate file library compilation (with redirect)..."
	echo -e "===========================================================\n"

	./debug/czhp-run -c samples/zhp/l1.cpp samples/zhp/l2.cpp -o samples/zhp/renamed.zhplib

	ls -la samples/zhp/*.zhplib

	# Display the symbols

	echo -e "\n==========================================================="
	echo "Displaying symbols in a single library..."
	echo -e "===========================================================\n"
	./debug/czhp-run -d samples/zhp/zhp_library.zhplib

	echo -e "\n==========================================================="
	echo "Displaying symbols in a multiple libraries..."
	echo -e "===========================================================\n"

	./debug/czhp-run -d samples/zhp/zhp_library.zhplib samples/zhp/renamed.zhplib samples/zhp/l1.zhplib

	# Run a sample script

	echo -e "\n==========================================================="
	echo "Running sample script for general features..."
	echo -e "===========================================================\n"

	./debug/czhp-run samples/zhp/simple.zhp

	# Run a sample script with importing

	echo -e "\n==========================================================="
	echo "Running sample script for import features..."
	echo -e "===========================================================\n"

	./debug/czhp-run samples/zhp/importing.zhp

	echo -e "\n==========================================================="
	echo "Running sample script for external import features..."
	echo -e "===========================================================\n"

	./debug/czhp-run samples/zhp/importing_external.zhp -L include

	echo -e "\n==========================================================="
	echo "Running czhp help guide..."
	echo -e "===========================================================\n"

	./debug/czhp-run -h
}

function czhp_simple {
	set +e

	mkdir -p bin

	# Run a sample script

	echo -e "\n==========================================================="
	echo "Running sample script for general features..."
	echo -e "===========================================================\n"

	./debug/czhp-run samples/zhp/simple.zhp -L include
}

function genlibs {
	mkdir -p include

	echo -e "==========================================================="
	echo "Compiling libraries..."
	echo -e "===========================================================\n"

	bin/czhp -v -c lib/io/io.cpp lib/io/formatted.cpp lib/io/file.cpp -o include/io.zhplib
	bin/czhp -v -c lib/math/math.cpp -o include/math.zhplib

	if ! [ -x "$(command -v tree)" ]; then
		ls include
	else
		tree include
	fi

	echo -e "\n==========================================================="
	echo "Displaying symbols..."
	echo -e "===========================================================\n"

	bin/czhp -d include/io.zhplib
	bin/czhp -d include/math.zhplib
}

# Ignore errors
set -e

if [ "$#" -eq 0 ]; then
	echo "Expected at least one argument"
elif [ $1 = "install" ]; then
	echo "Installing..."

	mkdir -p bin
	mkdir -p build

	compile_normal czhp
	compile_normal zhp-shared
	compile_normal zhp-static

	# Move executables and libraries
	if [ -f "libzhp.so" ]; then
		mv libzhp.* bin/
	fi

	# Place the binaries into their appropriate directories
	# [ -f zhetapi ] && mv zhetapi bin
	[ -f czhp ] && mv czhp bin

	# [ -f port ] && mv port build
	# [ -f exp ] && mv exp build
	# [ -f cuda ] && mv cuda build

	genlibs
elif [ $1 = "genlibs" ]; then
	genlibs
elif [ $1 = "port" ]; then
	# Run cmake
	cmake .

	mkdir -p bin

	# Compile and move apps
	make -j8 port

	mv port bin/

	./bin/port
elif [ $1 = "cuda-memcheck" ]; then
	compile_debug $2

	cuda-memcheck --show-backtrace yes --racecheck-report all --leak-check full ./debug/$2-debug
elif [ $1 = "cuda-gdb" ]; then
	compile_debug $2

	cuda-gdb ./debug/$2-debug
elif [ $1 = "gdb" ]; then
	compile_debug $2

	gdb debug/$2-debug
elif [ $1 = "valgrind" ] || [ $1 = "vgrind" ]; then
	compile_debug $2

	valgrind --leak-check=full --show-leak-kinds=all ./debug/$2-debug
elif [ $1 = "callgrind" ] || [ $1 = "kgrind" ]; then
	compile_debug $2

	valgrind --tool=callgrind --callgrind-out-file=cgrind.out ./debug/$2-run

	kcachegrind cgrind.out

	rm cgrind.out
else
	mkdir -p debug

	compile_normal $1

	mv $1 debug/$1-run

	if [ "$1" == "czhp" ]; then
		czhp_simple
	elif [ "$1" == "czhp_full" ]; then
		czhp_normal
	else
		./debug/$1-run
	fi
fi

if [ -f libzhp.a ]; then
	mv libzhp.a debug
fi

if [ -f libzhp.so ]; then
	mv libzhp.so debug
fi