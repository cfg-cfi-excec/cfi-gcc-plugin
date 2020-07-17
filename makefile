GCCDIR = /home/mario/gcc-plugins/gcc-install/bin


CXX = $(GCCDIR)/g++
CCX_RISCV32 = /opt/riscv/bin/riscv32-unknown-elf-gcc
CC_RISCV32 = /opt/riscv/lib/gcc/riscv32-unknown-elf/7.1.1/plugin/include

# Flags for the C++ compiler: C++11 and all the warnings
CXXFLAGS = -std=c++11 -Wall -fno-rtti -fPIC
# Workaround for an issue of -std=c++11 and the current GCC headers
CXXFLAGS += -Wno-literal-suffix

# Determine the plugin-dir and add it to the flags
PLUGINDIR=$(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -O -g -I$(CC_RISCV32)

LDFLAGS = -std=c++11

ROOT_DIR = ./
INCLUDE_DIR = -I$(ROOT_DIR)

# top level goal: build our plugin as a shared library
all: gcc_plugin.so

gcc_plugin.so: gcc_plugin_hcfi.o gcc_plugin.o main.o 
	$(CXX) $(LDFLAGS) -shared $^ -o $@

$(ROOT_DIR)/%.o: $(ROOT_DIR)/%.cpp
	$(CXX) $(INCLUDE_DIR) $(CXXFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f *.so
	rm -f *.o
	rm -f ./implementations/*.o
 
.PHONY: all clean check
