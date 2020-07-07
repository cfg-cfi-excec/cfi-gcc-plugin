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

PLUGIN_NAME = gcc_plugin_hcfi

# top level goal: build our plugin as a shared library
all: $(PLUGIN_NAME).so

$(PLUGIN_NAME).so: $(PLUGIN_NAME).o
	$(CXX) $(LDFLAGS) -shared -o $@ $<

$(PLUGIN_NAME).o : $(PLUGIN_NAME).cc 
	$(CXX) $(CXXFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f $(PLUGIN_NAME).o $(PLUGIN_NAME).so test

check: $(PLUGIN_NAME).so test.cc
	$(CCX_RISCV32) -march=rv32imfcxpulpv2 -mfdiv -D__riscv__ -O -fplugin=./$(PLUGIN_NAME).so -c test.cc -o test
 
.PHONY: all clean check
