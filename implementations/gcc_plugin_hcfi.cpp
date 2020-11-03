#include "gcc_plugin_hcfi.h"

  GCC_PLUGIN_HCFI::GCC_PLUGIN_HCFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_HCFI::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of __main with a CHECKLABEL
    if (strcmp(function_name.c_str(), "__main") == 0) {
      // reset CFI state (e.g., exit(1) might have left CFI module in a dirty state)
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
      // enable CFI from here on
      generateAndEmitAsm(CFI_ENABLE, firstInsn, firstBlock, false);
    } else if (strcmp(function_name.c_str(), "exit") == 0) {
      // reset CFI state because exit() breaks out of CFG
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
    } else {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      // only instrument functions, which are known to be called indirectly
      if (label >= 0) {
        generateAndEmitAsm("CHECKLABEL " + std::to_string(label), firstInsn, firstBlock, false);
      }
    }
  }
  
  void GCC_PLUGIN_HCFI::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_HCFI::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function return of __main
    // TODO: check if we can get rid of isFunctionExcludedFromCFI check here
    if (function_name.compare("__main") != 0 && !isFunctionExcludedFromCFI(function_name)) {
      generateAndEmitAsm("CHECKPC", lastInsn, lastBlock, false);
    } else {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_HCFI::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    // TODO: remove exclusion list here (tmp fix for soft fp lib functions)
    if(!isFunctionExcludedFromCFI(function_name)) {
      generateAndEmitAsm("SETPC", insn, block, false);
    } else {
      // TODO: add dummy instructions to match number of injected instructions
    }
  }

  void GCC_PLUGIN_HCFI::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    if (label >= 0) {
      generateAndEmitAsm("SETPCLABEL " + std::to_string(label), insn, block, false);
    } else {
      generateAndEmitAsm("SETPC", insn, block, false);
    }
  }

  void GCC_PLUGIN_HCFI::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HCFI::onSetJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
    generateAndEmitAsm("SJCFI " + std::to_string(readLabelFromTmpFile()), insn, block, false);
  }

  void GCC_PLUGIN_HCFI::onLongJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    generateAndEmitAsm("LJCFI", insn, block, false);
  }

  void GCC_PLUGIN_HCFI::init()
  {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        //std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //prinExistingFunctions();
        //printFunctionCalls();
      }
    }
  }

  GCC_PLUGIN_HCFI *GCC_PLUGIN_HCFI::clone()
  {
    // We do not clone ourselves
    return this;
  }