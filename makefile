GCCDIR = /home/mario/gcc-plugins/gcc-install/bin


CXX = $(GCCDIR)/g++
CCX_RISCV32 = /opt/riscv/bin/riscv32-unknown-elf-g++
CC_RISCV32 = /opt/riscv/lib/gcc/riscv32-unknown-elf/7.1.1/plugin/include

# Flags for the C++ compiler: C++11 and all the warnings
CXXFLAGS = -std=c++11 -Wall -fno-rtti -fPIC
# Workaround for an issue of -std=c++11 and the current GCC headers
CXXFLAGS += -Wno-literal-suffix

# Determine the plugin-dir and add it to the flags
PLUGINDIR=$(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -I$(CC_RISCV32)

LDFLAGS = -std=c++11

# top level goal: build our plugin as a shared library
all: warn_unused.so

warn_unused.so: warn_unused.o
	$(CXX) $(LDFLAGS) -shared -o $@ $<

warn_unused.o : warn_unused.cc
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f warn_unused.o warn_unused.so

check: warn_unused.so test.cc
	$(CCX_RISCV32) -fplugin=./warn_unused.so -c test.cc -o /dev/null 2> test.dot
	dot -Tpdf test.dot > test.pdf
	xdg-open test.pdf
 
.PHONY: all clean check
