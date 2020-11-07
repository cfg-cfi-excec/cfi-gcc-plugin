RISCV ?= /opt/riscv
$(info Using $(RISCV) as toolchain base dir)

CCX_RISCV32 = $(RISCV)/bin/riscv32-unknown-elf-gcc
CC_RISCV32 = $(RISCV)/lib/gcc/riscv32-unknown-elf/7.1.1/plugin/include

# Flags for the C++ compiler: C++11 and all the warnings
CXXFLAGS = -std=c++11 -Wall -fno-rtti -fPIC
# Workaround for an issue of -std=c++11 and the current GCC headers
CXXFLAGS += -Wno-literal-suffix

# Determine the plugin-dir and add it to the flags
PLUGINDIR=$(shell $(CXX) -print-file-name=plugin)
CXXFLAGS += -O -g -I$(CC_RISCV32)

LDFLAGS = -std=c++11

INCLUDE_DIR = -I./ -I./implementations

# top level goal: build our plugin as a shared library
all: gcc_plugin.so

gcc_plugin.so: ./asmgen/InstrType.o ./asmgen/UpdatePoint.o  ./asmgen/AsmGen.o ./implementations/gcc_plugin_icet.o ./implementations/gcc_plugin_hcfi.o ./implementations/gcc_plugin_excec.o ./implementations/gcc_plugin_hafix.o ./implementations/gcc_plugin_hecfi.o ./implementations/gcc_plugin_fixer.o gcc_plugin.o main.o 
	$(CXX) $(LDFLAGS) -shared $^ -o $@

%.o: %.cpp
	$(CXX) $(INCLUDE_DIR) $(CXXFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f *.so *.o ./implementations/*.o ./asmgen/*.o
 
.PHONY: all clean
