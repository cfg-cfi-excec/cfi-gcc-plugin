#include "gcc_plugin_icet.h"

  GCC_PLUGIN_ICET::GCC_PLUGIN_ICET(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
 
  }

  void GCC_PLUGIN_ICET::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of _main with a CFICHK
    if (strcmp(function_name.c_str(), "_main") == 0) {
      // reset CFI state (e.g., exit(1) might have left CFI module in a dirty state)
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
      // enable CFI from here on
      generateAndEmitAsm(CFI_ENABLE, firstInsn, firstBlock, false);
    } else if (strcmp(function_name.c_str(), "exit") == 0) {
      // reset CFI state because exit() breaks out of CFG
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
    } else {
      // TODO: replace with ENDBRANCH
      generateAndEmitAsm(ENDBRANCH, firstInsn, firstBlock, false);
    }
  }

  void GCC_PLUGIN_ICET::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    if (function_name.compare("_main") == 0) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_ICET::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    generateAndEmitAsm(ENDBRANCH, insn, block, false);
  }

  GCC_PLUGIN_ICET *GCC_PLUGIN_ICET::clone()
  {
    // We do not clone ourselves
    return this;
  }