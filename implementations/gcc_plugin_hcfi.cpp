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
    if (function_name.compare("__main") == 0) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
    } else if (!isExcludedFromBackwardEdgeCfi(function_name)) {
      // TODO: find out why exactly this hack is needed
      if (function_name.rfind("plp_udma_enqueue", 0) == 0) {
        generateAndEmitAsm("NOP", lastInsn, lastBlock, false);
      }

      generateAndEmitAsm("CHECKPC", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_HCFI::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    if(!isLibGccFunction(function_name)) {
      generateAndEmitAsm("SETPC", insn, block, false);
    } else {
      // Instead, add NOPs such that the runtime & code size is the same.
      // Two NOPs are added because backward edge cannot be instrumented.
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
    }
  }

  void GCC_PLUGIN_HCFI::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    if (label >= 0) {
      generateAndEmitAsm("SETPCLABEL " + std::to_string(label), insn, block, false);
    } else if (!isLibGccFunction(function_name)) {
      generateAndEmitAsm("SETPC", insn, block, false);
      handleIndirectFunctionCallWithoutConfigEntry(file_name, function_name, line_number);
    } else {
      // Instead, add NOPs such that the runtime & code size is the same.
      // Two NOPs are added because backward edge cannot be instrumented.
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
    }
  }

  void GCC_PLUGIN_HCFI::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_HCFI::onSetJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index) {
    // setjmp needs to be instrumented as well
    onDirectFunctionCall(file_name, function_name, block, insn);
    writeLabelToTmpFile(readLabelFromTmpFile()+1);
      
    rtx_insn *tmpInsn = NEXT_INSN(insn);
    while (NOTE_P(tmpInsn)) {
      tmpInsn = NEXT_INSN(tmpInsn);
    }

    // place cfisetjmp below actual setjmp() call
    // this is the place where longjmp() jumps to
    generateAndEmitAsm("SJCFI " + std::to_string(readLabelFromTmpFile()), tmpInsn, block, false);
  }

  void GCC_PLUGIN_HCFI::onLongJumpFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn, int index) {
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

        break;
      }
    }
  }

  GCC_PLUGIN_HCFI *GCC_PLUGIN_HCFI::clone()
  {
    // We do not clone ourselves
    return this;
  }