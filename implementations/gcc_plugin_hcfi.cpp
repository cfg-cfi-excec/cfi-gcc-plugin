#include "gcc_plugin_hcfi.h"

  GCC_PLUGIN_HCFI::GCC_PLUGIN_HCFI(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_HCFI::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of _main with a CHECKLABEL
    if (strcmp(function_name.c_str(), "_main") != 0) {
      // Disable CFI temporarily during printf
      // TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      // TODO: Fix CFI in printf
      if (function_name.compare("printf") == 0) {
        generateAndEmitAsm("CFI_DBG7 t0", firstInsn, firstBlock, false);
      }

      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      // only instrument functions, which are known to be called indirectly
      if (label >= 0) {
        generateAndEmitAsm("CHECKLABEL " + std::to_string(label), firstInsn, firstBlock, false);
      }
    } else {
      // enable CFI from here on
      //TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      generateAndEmitAsm("CFI_DBG6 t0", firstInsn, firstBlock, false);
    }
  }
  
  void GCC_PLUGIN_HCFI::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_HCFI::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function return of _main
    if (function_name.compare("_main") != 0) {
      // Enable CFI again after printf
      // TODO: replace CFI_DBG6 with some sort of CFI_ENABLE instruction
      // TODO: Fix CFI in printf
      if (function_name.compare("printf") == 0) {
        generateAndEmitAsm("CFI_DBG6 t0", lastInsn, lastBlock, false);
      }

      generateAndEmitAsm("CHECKPC", lastInsn, lastBlock, false);
    } else {
      // disable CFI from here on
      //TODO: replace CFI_DBG7 with some sort of CFI_DISABLE instruction
      generateAndEmitAsm("CFI_DBG7 t0", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_HCFI::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    // TODO: remove exclusion list here (tmp fix for soft fp lib functions)
    if(!isFunctionExcludedFromCFI(function_name)) {
      generateAndEmitAsm("SETPC", insn, block, false);
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
    //TODO: Set Label propperly
    generateAndEmitAsm("SJCFI 0x42", insn, block, false);
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