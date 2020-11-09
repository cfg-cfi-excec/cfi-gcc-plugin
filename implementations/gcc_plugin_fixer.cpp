#include "gcc_plugin_fixer.h"

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    if (strcmp(function_name.c_str(), "__main") == 0) {
      // reset CFI state (e.g., exit(1) might have left CFI module in a dirty state)
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
      // enable CFI from here on
      generateAndEmitAsm(CFI_ENABLE, firstInsn, firstBlock, false);

      // Load the policy matrix on entry of __main
      std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();
      for(CFG_FUNCTION_CALL function_call : function_calls) {
        for(CFG_SYMBOL call : function_call.calls) {
          generateAndEmitAsm("CFIMATLDCALLER " + std::to_string(function_call.label), firstInsn, firstBlock, false);
          generateAndEmitAsm("CFIMATLDCALLEE " + call.symbol_name, firstInsn, firstBlock, false);
        }
      }
    } else if (strcmp(function_name.c_str(), "exit") == 0) {
      // reset CFI state because exit() breaks out of CFG
      generateAndEmitAsm(CFI_RESET, firstInsn, firstBlock, false);
    }
  }
  
  void GCC_PLUGIN_FIXER::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_FIXER::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function return of __main
    if (function_name.compare("__main") == 0) {
      // disable CFI from here on
      generateAndEmitAsm(CFI_DISABLE, lastInsn, lastBlock, false);
    } else if (!isExcludedFromBackwardEdgeCfi(function_name)) {
      // BNE is required for creating the same performance overhead as original FIXER approach (branch never taken)
      //generateAndEmitAsm("BNE zero,zero,exit", lastInsn, lastBlock, false);
      generateAndEmitAsm("CFIRET", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    // Don't instrument function calls of __main and functions in libgcc
    if (function_name.compare("__main") != 0 && !isLibGccFunction(function_name)) {
      // The first two instructions are only needed to match the number of instructions in the original FIXER approach
      //generateAndEmitAsm("AUIPC t0,0", insn, block, false);
      //generateAndEmitAsm("ADD t0,t0,14", insn, block, false);
      generateAndEmitAsm("CFICALL", insn, block, false);
    } else {
      // Instead, add NOPs such that the runtime & code size is the same.
      // Two NOPs are added because backward edge cannot be instrumented.
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
      generateAndEmitAsm("add zero,zero,zero", insn, block, false);
    }
  }

  void GCC_PLUGIN_FIXER::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_FIXER::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
    int label = getLabelForIndirectFunctionCall(function_name, file_name, line_number);
    if (label >= 0) {
      // Only emit CFIFWD in case there is an entry in the config file
      rtx body = PATTERN(insn);
      rtx parallel = XVECEXP(body, 0, 0);
      rtx call;
      if (GET_CODE (parallel) == SET) {
        call = XEXP(parallel, 1);
      } else if (GET_CODE (parallel) == CALL) {
        call = parallel;
      }
      
      rtx mem = XEXP(call, 0);
      rtx subExpr = XEXP(mem, 0);

      if (((rtx_code)subExpr->code) == REG) {
        std::string regName = getRegisterNameForNumber(REGNO(subExpr));
        // The first two instructions are only needed to match the number of instructions in the original FIXER approach
        //generateAndEmitAsm("AUIPC t0,0", insn, block, false);
        //generateAndEmitAsm("ADD t0,t0,14", insn, block, false);
        // BNE is required for creating the same performance overhead as original FIXER approach (branch never taken)
        //generateAndEmitAsm("BNE zero,zero,exit", insn, block, false);
        generateAndEmitAsm("CFIFWD " + regName + ", " + std::to_string(label), insn, block, false);
      }
    } else if (!isExcludedFromForwardEdgeCfi(function_name)) {
      onDirectFunctionCall(file_name, function_name, block, insn);
    }
  }

  void GCC_PLUGIN_FIXER::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cerr << "CFG file for instrumentation: " << argv[i].value << "\n";

        readConfigFile(argv[i].value);
        //printFunctionCalls();
        //printLabelJumps();
        //printIndirectlyCalledFunctions();
      }
    }
  }

  GCC_PLUGIN_FIXER *GCC_PLUGIN_FIXER::clone()
  {
    // We do not clone ourselves
    return this;
  }