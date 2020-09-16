#include "gcc_plugin_excec.h"

  GCC_PLUGIN_EXCEC::GCC_PLUGIN_EXCEC(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_EXCEC::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Don't instrument function entry of MAIN
    if (strcmp(function_name.c_str(), "main") != 0) {
      int label = getLabelForIndirectlyCalledFunction(function_name, file_name);

      // only instrument functions, which are known to be called indirectly
      if (label >= 0) {
        generateAndEmitAsm("CFICHK " + std::to_string(label), firstInsn, firstBlock, false);
      }
    }
  }

  void GCC_PLUGIN_EXCEC::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function entry of MAIN
    if (function_name.compare("main") != 0) {
      generateAndEmitAsm("CFIRET", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_EXCEC::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    //TODO: change to custom instruction
    generateAndEmitAsm("CFICALL", insn, block, false);
  }

  void GCC_PLUGIN_EXCEC::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    if (label >= 0) {
      //TODO: change to custom instruction
      generateAndEmitAsm("CFIPRC " + std::to_string(label), insn, block, false);
    } else {
      //TODO: change to custom instruction
      generateAndEmitAsm("SETPC", insn, block, false);
    }
  }

  void GCC_PLUGIN_EXCEC::onNamedLabel(std::string file_name, std::string function_name, std::string label_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJumpSymbol(file_name, function_name, label_name);

    if (label >= 0) {
      generateAndEmitAsm("CFICHK " + std::to_string(label), insn, block, false);
    }
  }
  
  void GCC_PLUGIN_EXCEC::onIndirectJump(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectJump(file_name, function_name);

    if (label >= 0) {
      //TODO: change to custom instruction
      generateAndEmitAsm("CFIPRJ " + std::to_string(label), insn, block, false);
    } else {
       printf("\033[31m Warning: NO CFI RULES FOR INDIRECT JUMP IN %s:%s \x1b[0m\n",file_name.c_str(), function_name.c_str());
    }
  }

  void GCC_PLUGIN_EXCEC::init()
  {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //prinExistingFunctions();
        //printFunctionCalls();
      }
    }
  }

  GCC_PLUGIN_EXCEC *GCC_PLUGIN_EXCEC::clone()
  {
    // We do not clone ourselves
    return this;
  }