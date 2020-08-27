#include "gcc_plugin_fixer.h"

  GCC_PLUGIN_FIXER::GCC_PLUGIN_FIXER(gcc::context *ctxt, struct plugin_argument *arguments, int argcounter)
      : GCC_PLUGIN(ctxt, arguments, argcounter) {
    init();
  }

  void GCC_PLUGIN_FIXER::onFunctionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    // Load the policy matrix on entry of MAIN
    if (function_name.compare("main") == 0) {
      std::vector<CFG_FUNCTION_CALL> function_calls = getIndirectFunctionCalls();
      for(CFG_FUNCTION_CALL function_call : function_calls) {
        for(CFG_SYMBOL call : function_call.calls) {
          generateAndEmitAsm("CFIMATLDCALLER " + function_call.function_name + " +" + std::to_string(function_call.offset), firstInsn, firstBlock, false);
          generateAndEmitAsm("CFIMATLDCALLEE " + call.symbol_name, firstInsn, firstBlock, false);
        }
      }
    }
  }
  
  void GCC_PLUGIN_FIXER::onFunctionRecursionEntry(std::string file_name, std::string function_name, basic_block firstBlock, rtx_insn *firstInsn) {
    onFunctionEntry(file_name, function_name, firstBlock, firstInsn);
  }

  void GCC_PLUGIN_FIXER::onFunctionReturn(std::string file_name, std::string function_name, basic_block lastBlock, rtx_insn *lastInsn) {
    // Don't instrument function return of MAIN
    if (function_name.compare("main") != 0) {
      generateAndEmitAsm("CFIRET", lastInsn, lastBlock, false);
    }
  }

  void GCC_PLUGIN_FIXER::onDirectFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    generateAndEmitAsm("CFICALL", insn, block, false);
  }

  void GCC_PLUGIN_FIXER::onRecursiveFunctionCall(std::string file_name, std::string function_name, basic_block block, rtx_insn *insn) {
    onDirectFunctionCall(file_name, function_name, block, insn);
  }

  void GCC_PLUGIN_FIXER::onIndirectFunctionCall(std::string file_name, std::string function_name, int line_number, basic_block block, rtx_insn *insn) {
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
      insn = generateAndEmitAsm("CFIFWD " + regName, insn, block, false);
    }
  }

  void GCC_PLUGIN_FIXER::init() {
    for (int i = 0; i < argc; i++) {
      if (std::strcmp(argv[i].key, "cfg_file") == 0) {
        std::cout << "CFG file for instrumentation: " << argv[i].value << "\n";

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