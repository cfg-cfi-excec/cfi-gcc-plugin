#include "gcc_plugin_hafix.h"

  GCC_PLUGIN_HAFIX::GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {}

  void GCC_PLUGIN_HAFIX::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    if (strcmp(function_name.c_str(), "printf") == 0) {
      // Disable CFI temporarily during printf
      // TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      // TODO: Fix CFI in printf
      generateAndEmitAsm("CFI_DBG7 t0", firstInsn, firstBlock, false);
    } else if (strcmp(function_name.c_str(), "_main") == 0) {
      // enable CFI from here on
      //TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      generateAndEmitAsm("CFI_DBG6 t0", firstInsn, firstBlock, false);
    }

    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    generateAndEmitAsm("CFIBR " + std::to_string(readLabelFromTmpFile()), firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    generateAndEmitAsm("CFIREC " + std::to_string(readLabelFromTmpFile()), firstInsn, firstBlock, false);
  }

  void GCC_PLUGIN_HAFIX::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {

    if (function_name.compare("_main") != 0) {
      generateAndEmitAsm("CFIDEL " + std::to_string(readLabelFromTmpFile()), lastInsn, lastBlock, false);
    }

    if (function_name.compare("printf") == 0) {
      // Enable CFI again after printf
      // TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      // TODO: Fix CFI in printf
      generateAndEmitAsm("CFI_DBG6 t0", lastInsn, lastBlock, false);
    } else if (function_name.compare("_main") == 0) {
      // disable CFI from here on
      //TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      generateAndEmitAsm("CFI_DBG7 t0", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_HAFIX::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    // TODO: remove exclusion list here (tmp fix for soft fp lib functions)
    if (function_name.compare("printf") != 0 && !isFunctionExcludedFromCFI(function_name)) {
      // Don't generate CFIRET for printf
      // TODO: Fix CFI in printf
      generateAndEmitAsm("CFIRET " + std::to_string(readLabelFromTmpFile()), tmpInsn, block, false);
    }
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
