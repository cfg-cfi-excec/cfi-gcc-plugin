#include "gcc_plugin_hafix.h"

  GCC_PLUGIN_HAFIX::GCC_PLUGIN_HAFIX(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {}

  void GCC_PLUGIN_HAFIX::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    if (strcmp(function_name.c_str(), "__main") == 0) {
      // reset CFI state (e.g., exit(1) might have left CFI module in a dirty state)
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
      // enable CFI from here on
      generateAndEmitAsm(CFI_ENABLE, firstInsn, firstBlock, false);
    } else if (strcmp(function_name.c_str(), "exit") == 0) {
      // reset CFI state because exit() breaks out of CFG
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
    } 
    
    if (!isExcludedFromBackwardEdgeCfi(function_name)) {
      generateAndEmitAsm("CFIBR " + std::to_string(readLabelFromTmpFile()), firstInsn, firstBlock, false);
      writeLabelToTmpFile(readLabelFromTmpFile()+1);
    }
  }

  void GCC_PLUGIN_HAFIX::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    generateAndEmitAsm("CFIREC " + std::to_string(readLabelFromTmpFile()), firstInsn, firstBlock, false);
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
  }

  void GCC_PLUGIN_HAFIX::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {

    if (function_name.compare("__main") != 0 && !isExcludedFromBackwardEdgeCfi(function_name)) {
      generateAndEmitAsm("CFIDEL " + std::to_string(readLabelFromTmpFile()), lastInsn, lastBlock, false);
    }

    if (function_name.compare("__main") == 0) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
    }
  }
		
  void GCC_PLUGIN_HAFIX::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    if (!isLibGccFunction(function_name)) {
      generateAndEmitAsm("CFIRET " + std::to_string(readLabelFromTmpFile()), tmpInsn, block, false);
    } else {
      // Instead, add NOPs such that the runtime & code size is the same.
      // Two NOPs are added because backward edge cannot be instrumented.
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
    }
  }

  void GCC_PLUGIN_HAFIX::onSetJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int indexn) {
    // setjmp calls require normal instrumentation just as any other call
    onDirectFunctionCall(file_name, function_name, block, insn);
  }
	
  void GCC_PLUGIN_HAFIX::onLongJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index) {
    // lonjmp calls require normal instrumentation just as any other call
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HAFIX::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HAFIX::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  GCC_PLUGIN_HAFIX *GCC_PLUGIN_HAFIX::clone() {
    // We do not clone ourselves
    return this;
  }
