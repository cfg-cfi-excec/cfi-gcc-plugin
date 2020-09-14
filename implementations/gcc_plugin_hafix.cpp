#include "gcc_plugin_hafix.h"

  GCC_PLUGIN_HAFIX::GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {}

  void GCC_PLUGIN_HAFIX::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    generateAndEmitAsm("CFIBR " + std::to_string(readLabelFromTmpFile()), firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    generateAndEmitAsm("CFIREC " + std::to_string(readLabelFromTmpFile()), firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function returns of MAIN
    // This is a (dirty) hack because CFI enforcement requires CFIDEL to be followed by
    // CFIRET immediately, but functions above main are not instrumented.
    if (strcmp(function_name.c_str(), "main") != 0) {
      generateAndEmitAsm("CFIDEL " + std::to_string(readLabelFromTmpFile()), lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_HAFIX::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    generateAndEmitAsm("CFIRET " + std::to_string(readLabelFromTmpFile()), tmpInsn, block, false);
  }

  void GCC_PLUGIN_HAFIX::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HAFIX::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    generateAndEmitAsm("CFIRET " + std::to_string(readLabelFromTmpFile()), tmpInsn, block, false);
  }

  GCC_PLUGIN_HAFIX *GCC_PLUGIN_HAFIX::clone() {
    // We do not clone ourselves
    return this;
  }
